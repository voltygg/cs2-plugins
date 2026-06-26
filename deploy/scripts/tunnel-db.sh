#!/usr/bin/env bash
#
# SSH tunnel to the remote PostgreSQL behind a deployed VPS. Binds a local port
# (default 5433, to dodge a local postgres on 5432) to the remote 5432.
#
# Usage:
#   deploy/scripts/tunnel-db.sh --server box-a
#   deploy/scripts/tunnel-db.sh --host 203.0.113.10 [--db-host localhost]
#   deploy/scripts/tunnel-db.sh --server box-a --local-port 5544
#
# Then: psql "host=127.0.0.1 port=5433 dbname=admin_system user=cs2_app"

set -euo pipefail

ScriptDir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$ScriptDir/lib/common.sh"

ServerId=""
VpsHost="${VPS_HOST:-}"
SshUser="${SSH_USER:-}"
SshPort="${SSH_PORT:-}"
DbHost="${DB_HOST:-}"
DbPort="${DB_PORT:-}"
LocalPort="${LOCAL_PORT:-5433}"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --server)     ServerId="$2"; shift 2 ;;
        --host)       VpsHost="$2"; shift 2 ;;
        --ssh-user)   SshUser="$2"; shift 2 ;;
        --ssh-port)   SshPort="$2"; shift 2 ;;
        --db-host)    DbHost="$2"; shift 2 ;;
        --db-port)    DbPort="$2"; shift 2 ;;
        --local-port) LocalPort="$2"; shift 2 ;;
        -h|--help)    sed -n '2,11p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) die "unknown argument: $1" ;;
    esac
done

if [[ -n "$ServerId" ]]; then
    eval "$(inventory server-env "$ServerId")"
    eval "$(inventory db-conn)"
    VpsHost="${VpsHost:-$SRV_HOST}"
    SshUser="${SshUser:-$SRV_SSH_USER}"
    SshPort="${SshPort:-$SRV_SSH_PORT}"
    DbHost="${DbHost:-$DB_HOST}"
    DbPort="${DbPort:-$DB_PORT}"
fi

SshUser="${SshUser:-steam}"
SshPort="${SshPort:-22}"
DbHost="${DbHost:-localhost}"
DbPort="${DbPort:-5432}"

[[ -n "$VpsHost" ]] || die "VPS host is required (use --server <id> or --host <ip>)"

SshArgs=(-N -p "$SshPort" -L "127.0.0.1:$LocalPort:$DbHost:$DbPort")
if [[ -n "${SSH_OPTS:-}" ]]; then
    read -r -a ExtraSshArgs <<< "$SSH_OPTS"
    SshArgs+=("${ExtraSshArgs[@]}")
else
    # ExitOnForwardFailure: fail loudly if LocalPort is taken, don't connect without the forward.
    SshArgs+=(-o StrictHostKeyChecking=accept-new -o ServerAliveInterval=30 -o ExitOnForwardFailure=yes)
fi

echo "=== SSH tunnel: 127.0.0.1:$LocalPort -> $DbHost:$DbPort (via $SshUser@$VpsHost:$SshPort) ==="
echo "    connect: psql \"host=127.0.0.1 port=$LocalPort dbname=admin_system user=<db-user>\""
echo "    stop:    Ctrl-C"

exec ssh "${SshArgs[@]}" "$SshUser@$VpsHost"
