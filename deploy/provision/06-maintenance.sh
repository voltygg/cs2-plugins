#!/usr/bin/env bash
#
# Install the maintenance wrappers + timers: a daily SteamCMD update (and instance
# restart) and a daily config/DB backup. The backup dumps databases only on the box
# that has /etc/cs2-backup.env (see deploy/systemd/cs2-backup.env.example).

set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$HERE/lib/common.sh"
require_root

UnitsDir="$(cd "$HERE/../systemd" && pwd)"

log "Installing maintenance wrappers"
install -o root -g root -m 0755 "$UnitsDir/cs2-update.sh" "$CS2_DIR/cs2-update.sh"
install -o root -g root -m 0755 "$UnitsDir/cs2-backup.sh" "$CS2_DIR/cs2-backup.sh"

log "Installing timers"
for unit in cs2-update.service cs2-update.timer cs2-backup.service cs2-backup.timer; do
    install -m 0644 "$UnitsDir/$unit" "/etc/systemd/system/$unit"
done

systemctl daemon-reload
systemctl enable --now cs2-update.timer cs2-backup.timer
log "maintenance timers enabled"
systemctl list-timers 'cs2-*' --no-pager || true
