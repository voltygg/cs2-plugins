#!/usr/bin/env bash
set -euo pipefail

Root="/home/steam/cs2-dedicated"
Csgo="$Root/game/csgo"
AddonsSrc="/home/steam/plugin-bundles/addons"
MmsBase="${MMS_BASE:-https://mms.alliedmods.net/mmsdrop/2.0}"
MmsStamp="$Csgo/addons/metamod/.mms-build"

if [[ ! -d "$Csgo" ]]; then
    echo "CS2 game directory is not present yet: $Csgo" >&2
    exit 0
fi

if [[ -d "$AddonsSrc" ]]; then
    mkdir -p "$Csgo/addons"
    cp -a "$AddonsSrc/." "$Csgo/addons/"
fi

if [[ -f "$Csgo/gameinfo.gi" ]] && ! grep -q 'csgo/addons/metamod' "$Csgo/gameinfo.gi"; then
    awk '
        !done && $0 ~ /[[:space:]]Game[[:space:]]+csgo[[:space:]]*$/ {
            match($0, /^[[:space:]]*/); indent = substr($0, 1, RLENGTH)
            printf "%sGame\tcsgo/addons/metamod\n", indent
            done = 1
        }
        { print }
    ' "$Csgo/gameinfo.gi" > "$Csgo/gameinfo.gi.tmp"
    mv "$Csgo/gameinfo.gi.tmp" "$Csgo/gameinfo.gi"
    if ! grep -q 'csgo/addons/metamod' "$Csgo/gameinfo.gi"; then
        echo "WARNING: gameinfo.gi patch did not take (format changed?); metamod will NOT load" >&2
    fi
fi

# A CS2 update can retire symbols an older Metamod links against, so the build is
# re-checked on every launch instead of only when missing. Pin MMS_URL to opt out.
if [[ -z "${MMS_URL:-}" ]]; then
    latest="$(curl -fsSL "$MmsBase/mmsource-latest-linux" || true)"
    [[ -n "$latest" ]] && MMS_URL="$MmsBase/$latest"
fi

if [[ -z "${MMS_URL:-}" ]]; then
    if [[ ! -d "$Csgo/addons/metamod/bin" ]]; then
        echo "ERROR: Metamod is not installed and $MmsBase is unreachable" >&2
        exit 1
    fi
    echo "WARNING: could not reach $MmsBase; keeping the installed Metamod build" >&2
elif [[ ! -d "$Csgo/addons/metamod/bin" || "$MMS_URL" != "$(cat "$MmsStamp" 2>/dev/null)" ]]; then
    echo "Installing Metamod: $MMS_URL"
    tmp="$(mktemp -d)"
    curl -fsSL "$MMS_URL" -o "$tmp/mms.tar.gz"
    rm -rf "$Csgo/addons/metamod/bin"
    tar -xzf "$tmp/mms.tar.gz" -C "$Csgo"
    rm -rf "$tmp"
    printf '%s\n' "$MMS_URL" >"$MmsStamp"
fi
