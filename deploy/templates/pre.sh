#!/usr/bin/env bash
set -euo pipefail

Root="/home/steam/cs2-dedicated"
Csgo="$Root/game/csgo"
AddonsSrc="/home/steam/plugin-bundles/addons"
MmsBase="${MMS_BASE:-https://mms.alliedmods.net/mmsdrop/2.0}"

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
fi

if [[ ! -d "$Csgo/addons/metamod/bin" ]]; then
    if [[ -z "${MMS_URL:-}" ]]; then
        latest="$(curl -fsSL "$MmsBase/mmsource-latest-linux")"
        MMS_URL="$MmsBase/$latest"
    fi
    tmp="$(mktemp -d)"
    curl -fsSL "$MMS_URL" -o "$tmp/mms.tar.gz"
    tar -xzf "$tmp/mms.tar.gz" -C "$Csgo"
    rm -rf "$tmp"
fi
