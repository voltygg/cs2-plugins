#!/usr/bin/env bash
#
# One-time provisioning orchestrator for a fresh Ubuntu VPS. Runs the numbered
# steps in order; each is idempotent, so re-running is safe.
#
#   sudo deploy/provision/00-bootstrap.sh
#
# After this completes, install each box's plugins + instances with
# deploy/bin/deploy-remote.sh (which enables and starts the cs2-server@<name> units).

set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$HERE/lib/common.sh"
require_root

for step in 01-system 02-steamcmd-cs2 03-metamod 04-firewall 05-systemd 06-maintenance; do
    log "Step $step"
    bash "$HERE/$step.sh"
done

log "Provisioning complete."
echo
echo "Next steps:"
echo "  1) On your PostgreSQL host (once):  deploy/bin/ensure-databases.sh --admin-user postgres"
echo "  2) Deploy plugins + start instances: deploy/bin/deploy-remote.sh --server <id>"
