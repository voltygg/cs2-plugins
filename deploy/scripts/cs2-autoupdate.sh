#!/usr/bin/env bash
#
# Restart CS2 instances when the installed build trails the public branch, so
# SteamCMD (which only runs at container start) picks up Valve updates instead of
# the server rejecting clients with "client out of date". Run by a systemd timer.

set -euo pipefail

Cs2Root="${CS2_ROOT:-/home/steam/cs2}"
DeployRoot="${DEPLOY_ROOT:-$Cs2Root/deploy}"
AppId="${CS2_APP_ID:-730}"
AppManifest="${CS2_APPMANIFEST:-$Cs2Root/server/steamapps/appmanifest_$AppId.acf}"
SteamCmd="${STEAMCMD_SH:-/home/steam/steamcmd/steamcmd.sh}"

log() { printf '%s %s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$*"; }
warn() { log "WARN: $*" >&2; }

[[ -f "$DeployRoot/docker-compose.yml" ]] || { warn "no compose at $DeployRoot; skipping"; exit 0; }
cd "$DeployRoot"

# a running container to query SteamCMD through
Cid="$(docker compose ps -q --status running | head -n1 || true)"
[[ -n "$Cid" ]] || { warn "no running instance; skipping"; exit 0; }

Installed="$(grep -m1 '"buildid"' "$AppManifest" 2>/dev/null | grep -oE '[0-9]+' || true)"
[[ -n "$Installed" ]] || { warn "no installed buildid at $AppManifest; skipping"; exit 0; }

AppInfo="$(docker exec "$Cid" "$SteamCmd" \
    +login anonymous +app_info_update 1 +app_info_print "$AppId" +quit 2>/dev/null || true)"

# public buildid from the KeyValues dump; tokenize only strings/braces so the
# non-KV preamble is skipped
Latest="$(APPINFO="$AppInfo" APP_ID="$AppId" python3 <<'PY' || true
import os, sys

text = os.environ.get("APPINFO", "")
app = os.environ.get("APP_ID", "730")

tokens = []
i, n = 0, len(text)
while i < n:
    c = text[i]
    if c == '"':
        i += 1
        buf = []
        while i < n and text[i] != '"':
            if text[i] == "\\" and i + 1 < n:
                buf.append(text[i + 1]); i += 2; continue
            buf.append(text[i]); i += 1
        tokens.append(("s", "".join(buf))); i += 1
    elif c == "{":
        tokens.append(("{", None)); i += 1
    elif c == "}":
        tokens.append(("}", None)); i += 1
    else:
        i += 1

pos = 0
def table():
    global pos
    out = {}
    while pos < len(tokens):
        kind, val = tokens[pos]
        if kind == "}":
            pos += 1; return out
        if kind != "s":
            pos += 1; continue
        key = val; pos += 1
        if pos >= len(tokens):
            break
        k2, v2 = tokens[pos]
        if k2 == "{":
            pos += 1; out[key] = table()
        elif k2 == "s":
            out[key] = v2; pos += 1
        else:
            pos += 1
    return out

root = table()
try:
    print(root[app]["depots"]["branches"]["public"]["buildid"])
except (KeyError, TypeError):
    sys.exit(1)
PY
)"

# fail safe: never restart when the latest build is unknown
if [[ -z "$Latest" ]]; then
    warn "could not determine latest buildid; skipping (installed $Installed)"
    exit 0
fi

if [[ "$Installed" == "$Latest" ]]; then
    log "up to date (buildid $Installed)"
    exit 0
fi

log "update available: $Installed -> $Latest; restarting instances"
# one at a time: instances share one install, so avoid concurrent SteamCMD writes
for svc in $(docker compose ps --services); do
    log "restarting $svc"
    docker compose restart "$svc"
    cid="$(docker compose ps -q "$svc" || true)"
    [[ -n "$cid" ]] || continue
    sleep 10
    while docker exec "$cid" sh -c 'pgrep -f steamcmd >/dev/null 2>&1'; do
        log "$svc: steamcmd running; waiting"
        sleep 15
    done
    log "$svc: steamcmd idle"
done
log "restart complete (now buildid $Latest)"
