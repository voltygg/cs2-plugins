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

# Set SshTarget/SshArgs/RsyncSsh from SRV_* (call after `inventory server-env`).
build_ssh() {
    SshTarget="$SRV_SSH_USER@$SRV_HOST"
    SshArgs=(-p "$SRV_SSH_PORT")
    if [[ -n "${SSH_OPTS:-}" ]]; then
        local extra
        read -r -a extra <<< "$SSH_OPTS"
        SshArgs+=("${extra[@]}")
    else
        SshArgs+=(-o StrictHostKeyChecking=accept-new)
    fi

    RsyncSsh="ssh"
    local arg quoted
    for arg in "${SshArgs[@]}"; do
        printf -v quoted "%q" "$arg"
        RsyncSsh+=" $quoted"
    done
}

# Run a command on the server; honors DryRun.
runssh() {
    if [[ "${DryRun:-0}" -eq 1 ]]; then echo "DRY: ssh $SshTarget $*"; return 0; fi
    ssh "${SshArgs[@]}" "$SshTarget" "$@"
}

# rsync over the built ssh transport; honors DryRun.
do_rsync() {
    local extra=()
    [[ "${DryRun:-0}" -eq 1 ]] && extra+=(--dry-run --verbose)
    rsync -az "${extra[@]}" -e "$RsyncSsh" "$@"
}
