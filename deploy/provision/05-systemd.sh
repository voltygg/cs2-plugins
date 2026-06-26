#!/usr/bin/env bash
#
# Install the templated CS2 server unit + launch wrapper, and grant the deploy user
# passwordless control of the cs2-server@ instances (so deploy-remote.sh can
# enable/restart over SSH without an interactive sudo). Instances are NOT enabled
# here — deploy-remote.sh enables/starts them once it has pushed their env files.

set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$HERE/lib/common.sh"
require_root

UnitsDir="$(cd "$HERE/../systemd" && pwd)"
Systemctl="$(command -v systemctl || echo /usr/bin/systemctl)"

log "Installing cs2-server@.service and launch wrapper"
install -m 0644 "$UnitsDir/cs2-server@.service" /etc/systemd/system/cs2-server@.service
install -o root -g root -m 0755 "$UnitsDir/cs2-launch.sh" "$CS2_DIR/cs2-launch.sh"

log "Granting $STEAM_USER passwordless systemctl over cs2-server@ instances"
Sudoers="/etc/sudoers.d/cs2-deploy"
cat > "$Sudoers" <<EOF
$STEAM_USER ALL=(root) NOPASSWD: \
  $Systemctl start cs2-server@*, \
  $Systemctl stop cs2-server@*, \
  $Systemctl restart cs2-server@*, \
  $Systemctl enable cs2-server@*, \
  $Systemctl disable cs2-server@*
EOF
chmod 0440 "$Sudoers"
visudo -cf "$Sudoers" >/dev/null || { rm -f "$Sudoers"; die "generated sudoers was invalid"; }

systemctl daemon-reload
log "systemd unit installed (enable instances via deploy-remote.sh)"
