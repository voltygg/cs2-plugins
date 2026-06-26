#!/usr/bin/env bash
#
# One-time (idempotent) provisioning of the SHARED PostgreSQL instance:
# creates the app login role and one database per plugin (database-per-plugin),
# then grants the app role on each. Schemas/tables are NOT created here — each
# plugin's Migrator applies its own configs/migrations/ on first load.
#
# Run this once against the shared instance whenever you add a plugin (it skips
# anything that already exists). It is NOT part of per-box provisioning.
#
# Connection: host/port come from inventory.yml (database.host/port). You must
# supply a PostgreSQL SUPERUSER (or a role allowed to CREATEDB/CREATEROLE) to run
# the DDL, plus the app role's password.
#
# Usage:
#   APP_DB_PASSWORD=...  PGPASSWORD=<admin-pw> \
#     deploy/bin/ensure-databases.sh --admin-user postgres
#
# Env:
#   APP_DB_PASSWORD   (required) password to set on the app login role (matches SOPS DB_PASSWORD)
#   PGPASSWORD        (required) the admin/superuser password used to connect
#   PGHOST/PGPORT     optional overrides for the inventory host/port

set -euo pipefail

AdminUser="postgres"
while [[ $# -gt 0 ]]; do
    case "$1" in
        --admin-user) AdminUser="$2"; shift 2 ;;
        -h|--help) sed -n '2,30p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "Unknown argument: $1" >&2; exit 1 ;;
    esac
done

HereDir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYBIN="${PYBIN:-python3}"

if [[ -z "${APP_DB_PASSWORD:-}" ]]; then
    echo "ERROR: APP_DB_PASSWORD must be set (the app role's password)." >&2
    exit 1
fi
if [[ -z "${PGPASSWORD:-}" ]]; then
    echo "ERROR: PGPASSWORD must be set (the admin/superuser password)." >&2
    exit 1
fi

# Shared connection (DB_HOST/DB_PORT/DB_USER) from the inventory.
eval "$("$PYBIN" "$HereDir/inventory.py" db-conn)"
Host="${PGHOST:-$DB_HOST}"
Port="${PGPORT:-$DB_PORT}"
AppUser="$DB_USER"

psql_admin() {
    PGPASSWORD="$PGPASSWORD" psql -h "$Host" -p "$Port" -U "$AdminUser" \
        -d postgres -v ON_ERROR_STOP=1 -qtA "$@"
}

echo "=== Ensuring shared PostgreSQL on $Host:$Port (admin: $AdminUser) ==="

# App login role.
if [[ "$(psql_admin -c "SELECT 1 FROM pg_roles WHERE rolname='$AppUser'")" == "1" ]]; then
    echo "  role $AppUser: exists"
else
    psql_admin -c "CREATE ROLE \"$AppUser\" LOGIN PASSWORD '$APP_DB_PASSWORD'"
    echo "  role $AppUser: created"
fi

# One database per plugin.
while read -r plugin db; do
    [[ -n "$db" ]] || continue
    if [[ "$(psql_admin -c "SELECT 1 FROM pg_database WHERE datname='$db'")" == "1" ]]; then
        echo "  db $db ($plugin): exists"
    else
        psql_admin -c "CREATE DATABASE \"$db\" OWNER \"$AppUser\""
        echo "  db $db ($plugin): created"
    fi
    psql_admin -c "GRANT ALL PRIVILEGES ON DATABASE \"$db\" TO \"$AppUser\""
done < <("$PYBIN" "$HereDir/inventory.py" plugin-dbs)

echo "=== Done ==="
