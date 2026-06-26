#!/usr/bin/env bash
#
# Render one server's Docker Compose deployment from inventory.yml, rsync it to
# the box, and apply it with docker compose.

set -euo pipefail

ScriptDir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$ScriptDir/lib/common.sh"

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
        *) die "unknown argument: $1" ;;
    esac
done

[[ -n "$ServerId" ]] || die "--server <id> is required"

cd "$RepoRoot"

load_server_env "$DeployDir/secrets/servers/$ServerId"

eval "$(inventory server-env "$ServerId")"

RenderDir="$DeployDir/.render/$ServerId"
RenderArgs=(--server "$ServerId" --package-dir "$PackageDir" --out-dir "$RenderDir")
[[ -z "$RuntimeImage" ]] || RenderArgs+=(--runtime-image "$RuntimeImage")
"$PYBIN" "$ToolDir/render.py" "${RenderArgs[@]}"

SshTarget="$SRV_SSH_USER@$SRV_HOST"
RemoteRoot="$SRV_DEPLOY_ROOT"
printf -v RemoteRootQ "%q" "$RemoteRoot"

SshArgs=(-p "$SRV_SSH_PORT")
if [[ -n "${SSH_OPTS:-}" ]]; then
    read -r -a ExtraSshArgs <<< "$SSH_OPTS"
    SshArgs+=("${ExtraSshArgs[@]}")
else
    SshArgs+=(-o StrictHostKeyChecking=accept-new)
fi

RsyncSsh="ssh"
for arg in "${SshArgs[@]}"; do
    printf -v quoted "%q" "$arg"
    RsyncSsh+=" $quoted"
done

runssh() {
    if [[ "$DryRun" -eq 1 ]]; then echo "DRY: ssh $SshTarget $*"; return 0; fi
    ssh "${SshArgs[@]}" "$SshTarget" "$@"
}

do_rsync() {
    local extra=()
    [[ "$DryRun" -eq 1 ]] && extra+=(--dry-run --verbose)
    rsync -az "${extra[@]}" -e "$RsyncSsh" "$@"
}

echo "=== Deploying Dockerized CS2 to $ServerId ($SshTarget:$RemoteRoot) ==="
echo "    plugins:   ${SRV_PLUGINS:-<none>}"
echo "    instances: ${SRV_INSTANCES:-<none>}"

runssh "mkdir -p $RemoteRootQ"
do_rsync "$RenderDir/" "$SshTarget:$RemoteRoot/"

if [[ "$DryRun" -eq 1 ]]; then
    echo "=== Dry run complete; docker compose was not changed ==="
    exit 0
fi

runssh "cd $RemoteRootQ && docker compose pull && docker compose up -d --remove-orphans"

failed=0
for inst in $SRV_INSTANCES; do
    IFS=':' read -r name _port _map <<<"$inst"
    service="cs2-$name"
    printf -v service_q "%q" "$service"
    if runssh "cd $RemoteRootQ && docker compose ps --status running --services | grep -Fx $service_q >/dev/null"; then
        echo "    $service: running"
    else
        echo "    ERROR: $service is not running" >&2
        runssh "cd $RemoteRootQ && docker compose logs --tail=80 $service_q" || true
        failed=1
    fi
done

if [[ "$failed" -ne 0 ]]; then
    echo "=== Deploy to $ServerId FAILED ===" >&2
    exit 1
fi

echo "=== Deploy to $ServerId complete ==="
