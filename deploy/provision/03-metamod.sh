#!/usr/bin/env bash
#
# Install Metamod:Source 2.0 (Linux) into the CS2 install and patch gameinfo.gi so
# the engine loads it. Pin a specific build by setting MMS_URL; otherwise the latest
# 2.0 dev build is resolved from the AlliedModders "latest" pointer.

set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$HERE/lib/common.sh"
require_root

Csgo="$CS2_DIR/game/csgo"
Addons="$Csgo/addons"
Gameinfo="$Csgo/gameinfo.gi"
MmsBase="${MMS_BASE:-https://mms.alliedmods.net/mmsdrop/2.0}"

[[ -f "$Gameinfo" ]] || die "gameinfo.gi not found — run 02-steamcmd-cs2.sh first"

if [[ -d "$Addons/metamod" ]]; then
    log "Metamod already installed at $Addons/metamod"
else
    if [[ -z "${MMS_URL:-}" ]]; then
        latest="$(curl -sqL "$MmsBase/mmsource-latest-linux")" \
            || die "could not resolve latest Metamod build from $MmsBase"
        [[ -n "$latest" ]] || die "empty Metamod 'latest' pointer"
        MMS_URL="$MmsBase/$latest"
    fi
    log "Installing Metamod from $MMS_URL"
    tmp="$(mktemp -d)"
    curl -sqL "$MMS_URL" -o "$tmp/mms.tar.gz" || die "download failed: $MMS_URL"
    sudo -u "$STEAM_USER" tar -xzf "$tmp/mms.tar.gz" -C "$Csgo"
    rm -rf "$tmp"
    log "Metamod extracted into $Addons"
fi

patch_gameinfo "$Gameinfo"
