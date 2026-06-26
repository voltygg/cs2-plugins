#!/usr/bin/env bash
#
# Back up the plugin configs (always) and the shared PostgreSQL plugin databases
# (only where /etc/cs2-backup.env defines the connection — typically one box, or
# wherever Postgres is reachable). Run by cs2-backup.service. Prunes >14d backups.
#
# Installed to /home/steam/cs2/cs2-backup.sh.

set -euo pipefail

Dest="/home/steam/backups"
EnvFile="/etc/cs2-backup.env"
Csgo="/home/steam/cs2/game/csgo"

mkdir -p "$Dest"
ts="$(date +%Y%m%d-%H%M%S)"

if [[ -d "$Csgo/addons" ]]; then
    tar -czf "$Dest/configs-$ts.tar.gz" -C "$Csgo" addons
    echo "configs -> $Dest/configs-$ts.tar.gz"
fi

if [[ -f "$EnvFile" ]]; then
    # Expects: PGHOST PGPORT PGUSER PGPASSWORD CS2_DATABASES="admin_system match_system"
    set -a; source "$EnvFile"; set +a
    for db in ${CS2_DATABASES:-}; do
        PGPASSWORD="${PGPASSWORD:-}" pg_dump -h "${PGHOST:-localhost}" -p "${PGPORT:-5432}" \
            -U "${PGUSER:-cs2_app}" "$db" | gzip > "$Dest/db-$db-$ts.sql.gz"
        echo "db $db -> $Dest/db-$db-$ts.sql.gz"
    done
else
    echo "no $EnvFile — skipping DB dump (configs only)"
fi

find "$Dest" -type f -name '*.gz' -mtime +14 -delete 2>/dev/null || true
