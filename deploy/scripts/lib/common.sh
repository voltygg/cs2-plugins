#!/usr/bin/env bash

# Shared helpers for deploy/scripts/*. Source, do not execute.

DeployLibDir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DeployScriptDir="$(cd "$DeployLibDir/.." && pwd)"
DeployDir="$(cd "$DeployScriptDir/.." && pwd)"
RepoRoot="$(cd "$DeployDir/.." && pwd)"
ToolDir="$DeployDir/tools"
PYBIN="${PYBIN:-python3}"

die() {
    echo "ERROR: $*" >&2
    exit 1
}

inventory() {
    "$PYBIN" "$ToolDir/inventory.py" "$@"
}

load_server_env() {
    local dir="$1"
    [[ -f "$dir/.env" ]] || die "no env for $(basename "$dir") (expected $dir/.env)"
    set -a
    if ! source "$dir/.env"; then
        set +a
        die "failed to load $dir/.env"
    fi
    set +a
}
