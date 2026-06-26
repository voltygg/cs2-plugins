#!/usr/bin/env bash
#
# Configure ufw: allow SSH and the CS2 instance port range (UDP game traffic + TCP
# for RCON/usercon and GOTV). The range covers several instances on one box.

set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$HERE/lib/common.sh"
require_root

SshPort="${SSH_PORT:-22}"
PortRange="${CS2_PORT_RANGE:-27015:27035}"

log "Configuring ufw (SSH $SshPort, CS2 $PortRange)"
ufw default deny incoming
ufw default allow outgoing
ufw allow "${SshPort}/tcp"
ufw allow "${PortRange}/udp"
ufw allow "${PortRange}/tcp"
ufw --force enable
ufw status verbose || true
