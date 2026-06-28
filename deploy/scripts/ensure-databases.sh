#!/usr/bin/env bash
#
# Idempotent provisioning of the shared Postgres: creates the app login role and
# one database per plugin, then grants the app role on each. Run once per added
# plugin; plugins apply their own migrations on load. Needs a superuser.
#
# Local (connects to the inventory DB host; pass the passwords explicitly):
#   DB_PASSWORD=... PGPASSWORD=<admin-pw> ensure-databases.sh --admin-user postgres
# Remote (over SSH; reads SSH_KEY/DB_PASSWORD/PGPASSWORD from the server .env):
#   ensure-databases.sh --server box-a
#
# DB_PASSWORD: app role password.  PGPASSWORD: superuser password.
# PGHOST/PGPORT override host/port. Explicit env vars override .env values.

set -euo pipefail

ScriptDir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$ScriptDir/lib/common.sh"

AdminUser="postgres"
ServerId=""
Remote=0
DryRun=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --admin-user) AdminUser="$2"; shift 2 ;;
        --server)     ServerId="$2"; Remote=1; shift 2 ;;
        --dry-run)    DryRun=1; shift ;;
        -h|--help) sed -n '2,13p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) die "unknown argument: $1" ;;
    esac
done

if [[ "$Remote" -eq 1 ]]; then
    # Connect over SSH and read SSH_KEY/DB_PASSWORD/PGPASSWORD from the server .env
    eval "$(inventory server-env "$ServerId")"
    _db="${DB_PASSWORD:-}"; _pg="${PGPASSWORD:-}"
    EnvDir="$DeployDir/secrets/servers/$ServerId"

    [[ -f "$EnvDir/.env" ]] && load_server_env "$EnvDir"
    [[ -n "$_db" ]] && DB_PASSWORD="$_db"
    [[ -n "$_pg" ]] && PGPASSWORD="$_pg"
    build_ssh
fi

if [[ -z "${DB_PASSWORD:-}" ]]; then
    die "DB_PASSWORD must be set (the app role's password)."
fi
if [[ -z "${PGPASSWORD:-}" ]]; then
    die "PGPASSWORD must be set (the admin/superuser password)."
fi
# --dry-run only previews the over-SSH path; local mode would hit the real DB.
if [[ "$DryRun" -eq 1 && "$Remote" -eq 0 ]]; then
    die "--dry-run requires --server (it previews the over-SSH commands)."
fi

# Shared connection (DB_HOST/DB_PORT/DB_USER) from the inventory.
eval "$(inventory db-conn)"
AppUser="$DB_USER"
Port="${PGPORT:-$DB_PORT}"

# On the box, Postgres is at localhost (inventory's host.docker.internal only
# resolves inside a container).
if [[ "$Remote" -eq 1 ]]; then
    Host="${PGHOST:-localhost}"
else
    Host="${PGHOST:-$DB_HOST}"
fi

psql_admin() {
    if [[ "$Remote" -eq 1 ]]; then
        local cmd
        printf -v cmd 'PGPASSWORD=%q psql -h %q -p %q -U %q -d postgres -v ON_ERROR_STOP=1 -qtA' \
            "$PGPASSWORD" "$Host" "$Port" "$AdminUser"
        local arg quoted
        for arg in "$@"; do
            printf -v quoted ' %q' "$arg"
            cmd+="$quoted"
        done
        runssh "$cmd"
    else
        psql -h "$Host" -p "$Port" -U "$AdminUser" \
            -d postgres -v ON_ERROR_STOP=1 -qtA "$@"
    fi
}

echo "=== Ensuring shared PostgreSQL on $Host:$Port (admin: $AdminUser) ==="

# App login role.
if [[ "$(psql_admin -c "SELECT 1 FROM pg_roles WHERE rolname='$AppUser'")" == "1" ]]; then
    echo "  role $AppUser: exists"
else
    psql_admin -c "CREATE ROLE \"$AppUser\" LOGIN PASSWORD '$DB_PASSWORD'"
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
done < <(inventory plugin-dbs)

echo "=== Done ==="
