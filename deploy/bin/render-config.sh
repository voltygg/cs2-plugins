#!/usr/bin/env bash
#
# Render one plugin's settings.jsonc for one server by filling the per-plugin
# template with: the inventory DB connection (+ the plugin's own DB name) and the
# server's unified env file (non-secret values + secrets, .sops.env / .env).
#
# Usage:
#   render-config.sh <server-id> <plugin> [--out FILE]
#
# Default output: deploy/.render/<server-id>/<plugin>/settings.jsonc (gitignored).
# deploy-remote.sh passes --out to write straight into the deploy bundle.

set -euo pipefail

ServerId="${1:-}"
Plugin="${2:-}"
shift 2 2>/dev/null || true
OutFile=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --out) OutFile="$2"; shift 2 ;;
        *) echo "Unknown argument: $1" >&2; exit 1 ;;
    esac
done

if [[ -z "$ServerId" || -z "$Plugin" ]]; then
    echo "Usage: render-config.sh <server-id> <plugin> [--out FILE]" >&2
    exit 1
fi

HereDir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DeployDir="$(cd "$HereDir/.." && pwd)"
PYBIN="${PYBIN:-python3}"

Template="$DeployDir/config/plugins/$Plugin/settings.template.jsonc"
ServerDir="$DeployDir/config/servers/$ServerId"

if [[ ! -f "$Template" ]]; then
    echo "ERROR: no template at $Template" >&2
    exit 1
fi

# 1) DB connection + this plugin's DB name from the inventory.
eval "$("$PYBIN" "$HereDir/inventory.py" render-env "$ServerId" "$Plugin")"

# 2) Per-server values + secrets (unified .sops.env / .env), prefer the encrypted one.
source "$HereDir/lib/secrets.sh"
load_server_env "$ServerDir"

# Optional placeholders default to empty (fixedLink cheat-check mode leaves these blank).
: "${CHEAT_ROOM_URL:=}"
: "${CHEAT_API_KEY:=}"

# 3) Required placeholders must be non-empty.
for v in DB_HOST DB_PORT DB_NAME DB_USER DB_PASSWORD DB_SSLMODE; do
    [[ -n "${!v:-}" ]] || { echo "ERROR: required var $v is empty for $ServerId/$Plugin" >&2; exit 1; }
done

[[ -n "$OutFile" ]] || OutFile="$DeployDir/.render/$ServerId/$Plugin/settings.jsonc"
mkdir -p "$(dirname "$OutFile")"

# 4) Substitute ONLY our known placeholders (so a stray $var elsewhere is left alone
#    and caught by the leftover check below).
export DB_HOST DB_PORT DB_NAME DB_USER DB_PASSWORD DB_SSLMODE CHEAT_ROOM_URL CHEAT_API_KEY
Vars='${DB_HOST} ${DB_PORT} ${DB_NAME} ${DB_USER} ${DB_PASSWORD} ${DB_SSLMODE} ${CHEAT_ROOM_URL} ${CHEAT_API_KEY}'
envsubst "$Vars" < "$Template" > "$OutFile"

# 5) Validate on the comment-stripped body: no leftover placeholders, parses as JSON.
"$PYBIN" - "$OutFile" <<'PY'
import json, re, sys
raw = open(sys.argv[1], encoding="utf-8").read()
body = re.sub(r"(?m)^\s*//.*$", "", raw)   # drop full-line // comments (keeps URLs)
leftover = sorted(set(re.findall(r"\$\{[A-Za-z_][A-Za-z0-9_]*\}", body)))
if leftover:
    sys.exit("ERROR: unsubstituted placeholders: " + ", ".join(leftover))
json.loads(body)
PY

echo "rendered $ServerId/$Plugin -> $OutFile"
