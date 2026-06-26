#!/usr/bin/env bash
#
# Render one server's Docker Compose deployment from inventory.yml, rsync it to
# the box, and apply it with docker compose.

set -euo pipefail

ServerId=""
PackageDir="package"
DryRun=0
RuntimeImage="${RUNTIME_IMAGE:-}"
while [[ $# -gt 0 ]]; do
    case "$1" in
        --server)        ServerId="$2"; shift 2 ;;
        --package-dir)   PackageDir="$2"; shift 2 ;;
        --runtime-image) RuntimeImage="$2"; shift 2 ;;
        --dry-run)       DryRun=1; shift ;;
        -h|--help)       sed -n '2,14p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "Unknown argument: $1" >&2; exit 1 ;;
    esac
done

[[ -n "$ServerId" ]] || { echo "ERROR: --server <id> is required" >&2; exit 1; }

ScriptDir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DeployDir="$(cd "$ScriptDir/.." && pwd)"
ToolDir="$DeployDir/tools"
RepoRoot="$(cd "$DeployDir/.." && pwd)"
PYBIN="${PYBIN:-python3}"
cd "$RepoRoot"

source "$ScriptDir/lib/secrets.sh"
load_server_env "$DeployDir/secrets/servers/$ServerId"

eval "$("$PYBIN" "$ToolDir/inventory.py" server-env "$ServerId")"

RenderDir="$DeployDir/.render/$ServerId"
RenderArgs=(--server "$ServerId" --package-dir "$PackageDir" --out-dir "$RenderDir")
[[ -z "$RuntimeImage" ]] || RenderArgs+=(--runtime-image "$RuntimeImage")
"$PYBIN" "$ToolDir/render.py" "${RenderArgs[@]}"

SshOpts="${SSH_OPTS:--o StrictHostKeyChecking=accept-new}"
SshTarget="$SRV_SSH_USER@$SRV_HOST"
SshCmd="ssh -p $SRV_SSH_PORT $SshOpts"
RemoteRoot="$SRV_DEPLOY_ROOT"

runssh() {
    if [[ "$DryRun" -eq 1 ]]; then echo "DRY: ssh $SshTarget $*"; return 0; fi
    $SshCmd "$SshTarget" "$@"
}

do_rsync() {
    local extra=()
    [[ "$DryRun" -eq 1 ]] && extra+=(--dry-run --verbose)
    rsync -az "${extra[@]}" -e "$SshCmd" "$@"
}

echo "=== Deploying Dockerized CS2 to $ServerId ($SshTarget:$RemoteRoot) ==="
echo "    plugins:   ${SRV_PLUGINS:-<none>}"
echo "    instances: ${SRV_INSTANCES:-<none>}"

runssh "mkdir -p '$RemoteRoot'"
do_rsync "$RenderDir/" "$SshTarget:$RemoteRoot/"

if [[ "$DryRun" -eq 1 ]]; then
    echo "=== Dry run complete; docker compose was not changed ==="
    exit 0
fi

runssh "cd '$RemoteRoot' && docker compose pull && docker compose up -d --remove-orphans"

failed=0
for inst in $SRV_INSTANCES; do
    IFS=':' read -r name _port _map <<<"$inst"
    service="cs2-$name"
    if runssh "cd '$RemoteRoot' && docker compose ps --status running --services | grep -Fx '$service' >/dev/null"; then
        echo "    $service: running"
    else
        echo "    ERROR: $service is not running" >&2
        runssh "cd '$RemoteRoot' && docker compose logs --tail=80 '$service'" || true
        failed=1
    fi
done

if [[ "$failed" -ne 0 ]]; then
    echo "=== Deploy to $ServerId FAILED ===" >&2
    exit 1
fi

echo "=== Deploy to $ServerId complete ==="
