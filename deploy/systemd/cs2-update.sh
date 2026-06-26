#!/usr/bin/env bash
#
# Update the CS2 dedicated server via SteamCMD, then restart any running instances.
# Run by cs2-update.service (root). Installed to /home/steam/cs2/cs2-update.sh.

set -euo pipefail

SteamUser="steam"
SteamCmd="/home/steam/steamcmd/steamcmd.sh"
Cs2Dir="/home/steam/cs2"

echo "=== Updating CS2 (app 730) ==="
runuser -u "$SteamUser" -- "$SteamCmd" \
    +force_install_dir "$Cs2Dir" +login anonymous +app_update 730 validate +quit

echo "=== Restarting running CS2 instances ==="
# Glob matches only loaded cs2-server@<name> units; no-op if none are running.
systemctl restart 'cs2-server@*' || true
