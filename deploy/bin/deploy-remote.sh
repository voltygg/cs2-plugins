#!/usr/bin/env bash
#
# Deploy the packaged plugins assigned to ONE server (from inventory.yml) over SSH:
#   render per-server settings.jsonc -> rsync each plugin's addons/ -> push per-instance
#   env files -> enable/restart the systemd instances -> verify they are active.
#
# Run package-plugin.sh for each plugin first so package/<plugin>/ exists. This is the
# Linux/remote analog of scripts/deploy.sh (which stays the local Windows dev tool).
#
# Usage:
#   deploy-remote.sh --server <id> [--package-dir DIR] [--dry-run] [--preserve-settings]
#
# Env:
#   SSH_OPTS   extra ssh/rsync options (e.g. -i key, -o ...). Default adds accept-new host keys.

set -euo pipefail

ServerId=""
PackageDir="package"
DryRun=0
PreserveSettings=0
while [[ $# -gt 0 ]]; do
    case "$1" in
        --server)            ServerId="$2"; shift 2 ;;
        --package-dir)       PackageDir="$2"; shift 2 ;;
        --dry-run)           DryRun=1; shift ;;
        --preserve-settings) PreserveSettings=1; shift ;;
        -h|--help) sed -n '2,16p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "Unknown argument: $1" >&2; exit 1 ;;
    esac
done

[[ -n "$ServerId" ]] || { echo "ERROR: --server <id> is required" >&2; exit 1; }

HereDir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DeployDir="$(cd "$HereDir/.." && pwd)"
RepoRoot="$(cd "$DeployDir/.." && pwd)"
PYBIN="${PYBIN:-python3}"
cd "$RepoRoot"

source "$HereDir/lib/secrets.sh"

# Resolve the server (host, ssh, paths, plugins, instances).
eval "$("$PYBIN" "$HereDir/inventory.py" server-env "$ServerId")"

SshOpts="${SSH_OPTS:--o StrictHostKeyChecking=accept-new}"
SshTarget="$SRV_SSH_USER@$SRV_HOST"
SshCmd="ssh -p $SRV_SSH_PORT $SshOpts"

runssh() {
    if [[ "$DryRun" -eq 1 ]]; then echo "DRY: ssh $SshTarget $*"; return 0; fi
    $SshCmd "$SshTarget" "$@"
}
do_rsync() {
    local extra=()
    [[ "$DryRun" -eq 1 ]] && extra+=(--dry-run --verbose)
    rsync -az "${extra[@]}" -e "$SshCmd" "$@"
}

echo "=== Deploying to $ServerId ($SshTarget) ==="
echo "    plugins:   ${SRV_PLUGINS:-<none>}"
echo "    instances: ${SRV_INSTANCES:-<none>}"
echo

runssh "mkdir -p '$SRV_CSGO_PATH/addons' '$SRV_CS2_ROOT/instances'"

# --- Plugins: render config into the bundle, then rsync the bundle's addons/ tree. ---
for plugin in $SRV_PLUGINS; do
    bundle="$PackageDir/$plugin"
    [[ -d "$bundle/addons" ]] || { echo "ERROR: no bundle at $bundle (run package-plugin.sh $plugin)" >&2; exit 1; }

    if [[ "$PreserveSettings" -eq 1 ]]; then
        echo "--- $plugin (settings.jsonc preserved on server) ---"
    else
        echo "--- $plugin (rendering settings.jsonc) ---"
        bash "$HereDir/render-config.sh" "$ServerId" "$plugin" \
            --out "$bundle/addons/$plugin/configs/settings.jsonc"
    fi
    do_rsync "$bundle/addons/" "$SshTarget:$SRV_CSGO_PATH/addons/"
done

# --- Instances: render env files (map/port from inventory, GSLT/RCON from secrets). ---
load_server_env "$DeployDir/config/servers/$ServerId"
InstRenderDir="$DeployDir/.render/$ServerId/instances"
mkdir -p "$InstRenderDir"

names=()
for inst in $SRV_INSTANCES; do
    IFS=':' read -r name port map <<<"$inst"
    [[ -n "$name" ]] || continue
    names+=("$name")
    gslt_var="GSLT_${name}"; rcon_var="RCON_${name}"
    cat > "$InstRenderDir/$name.env" <<EOF
MAP=${map}
PORT=${port}
GSLT=${!gslt_var:-}
RCON=${!rcon_var:-}
EOF
done

if [[ ${#names[@]} -gt 0 ]]; then
    do_rsync "$InstRenderDir/" "$SshTarget:$SRV_CS2_ROOT/instances/"
fi

# --- Enable + restart instances, then verify. ---
failed=0
for name in "${names[@]}"; do
    echo "--- instance $name: enable + restart ---"
    runssh "sudo systemctl enable cs2-server@$name" || true
    runssh "sudo systemctl restart cs2-server@$name"
    if [[ "$DryRun" -eq 1 ]]; then continue; fi
    if runssh "systemctl is-active --quiet cs2-server@$name"; then
        echo "    cs2-server@$name: active"
    else
        echo "    ERROR: cs2-server@$name is NOT active" >&2
        runssh "sudo journalctl -u cs2-server@$name -n 30 --no-pager" || true
        failed=1
    fi
done

echo
if [[ "$failed" -ne 0 ]]; then
    echo "=== Deploy to $ServerId FAILED (an instance is not active) ===" >&2
    exit 1
fi
echo "=== Deploy to $ServerId complete ==="
