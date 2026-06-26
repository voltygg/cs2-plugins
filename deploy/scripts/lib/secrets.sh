# Shared per-server env loader for the deploy scripts. Source, don't execute.
#
# load_server_env <server-config-dir>
#   Exports the KEY=VALUE pairs from the server's unified env file (non-secret values
#   AND secrets), preferring the SOPS-encrypted .sops.env and falling back to a
#   plaintext .env (local dev only). Returns non-zero if neither exists.

load_server_env() {
    local dir="$1"
    if [[ -f "$dir/.sops.env" ]]; then
        command -v sops >/dev/null 2>&1 || { echo "ERROR: sops not installed" >&2; return 1; }
        set -a; source <(sops -d "$dir/.sops.env"); set +a
    elif [[ -f "$dir/.env" ]]; then
        echo "WARN: using plaintext $dir/.env (local dev only)" >&2
        set -a; source "$dir/.env"; set +a
    else
        echo "ERROR: no env for $(basename "$dir") (expected .sops.env)" >&2
        return 1
    fi
}
