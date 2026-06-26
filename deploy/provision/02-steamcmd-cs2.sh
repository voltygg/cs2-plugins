#!/usr/bin/env bash
#
# Install SteamCMD and the CS2 dedicated server (app 730). Re-running just updates
# CS2 to the latest build (validate repairs any changed files).

set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$HERE/lib/common.sh"
require_root

STEAMCMD_URL="${STEAMCMD_URL:-https://steamcdn-a.akamaihd.net/client/installer/steamcmd_linux.tar.gz}"

if [[ -f "$STEAMCMD_DIR/steamcmd.sh" ]]; then
    log "SteamCMD already installed"
else
    log "Installing SteamCMD into $STEAMCMD_DIR"
    curl -sqL "$STEAMCMD_URL" | sudo -u "$STEAM_USER" tar -xz -C "$STEAMCMD_DIR"
fi

log "Installing/updating CS2 dedicated server (app 730) — this can take a while"
sudo -u "$STEAM_USER" "$STEAMCMD_DIR/steamcmd.sh" \
    +force_install_dir "$CS2_DIR" \
    +login anonymous \
    +app_update 730 validate \
    +quit

[[ -f "$CS2_DIR/game/bin/linuxsteamrt64/cs2" ]] \
    || die "CS2 binary missing after install — check SteamCMD output above"
log "CS2 dedicated server installed at $CS2_DIR"
