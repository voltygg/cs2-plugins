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
    if [[ -f "$dir/.sops.env" ]]; then
        command -v sops >/dev/null 2>&1 || die "sops not installed"
        local tmp
        tmp="$(mktemp)"
        if ! sops -d "$dir/.sops.env" > "$tmp"; then
            rm -f "$tmp"
            die "failed to decrypt $dir/.sops.env"
        fi
        set -a
        if ! source "$tmp"; then
            set +a
            rm -f "$tmp"
            die "failed to load decrypted env from $dir/.sops.env"
        fi
        set +a
        rm -f "$tmp"
    elif [[ -f "$dir/.env" ]]; then
        echo "WARN: using plaintext $dir/.env (local dev only)" >&2
        set -a
        if ! source "$dir/.env"; then
            set +a
            die "failed to load $dir/.env"
        fi
        set +a
    else
        die "no env for $(basename "$dir") (expected .sops.env)"
    fi
}
