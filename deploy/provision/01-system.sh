#!/usr/bin/env bash
#
# Base system prep: 32-bit libs (SteamCMD/CS2 need them), tooling, and the
# unprivileged `steam` user with its directory skeleton.

set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$HERE/lib/common.sh"
require_root

log "Enabling i386 architecture and installing packages"
dpkg --add-architecture i386
apt-get update -y
DEBIAN_FRONTEND=noninteractive apt-get install -y \
    ca-certificates curl tar gzip \
    lib32gcc-s1 libc6-i386 lib32stdc++6 \
    ufw cron rsync sudo postgresql-client

if id -u "$STEAM_USER" >/dev/null 2>&1; then
    log "user '$STEAM_USER' already exists"
else
    useradd -m -s /bin/bash "$STEAM_USER"
    log "created user '$STEAM_USER'"
fi

install -d -o "$STEAM_USER" -g "$STEAM_USER" \
    "$CS2_DIR" "$CS2_DIR/instances" "$STEAMCMD_DIR" "$BACKUP_DIR"
log "directory skeleton ready under $STEAM_HOME"
