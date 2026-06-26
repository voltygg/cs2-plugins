#!/usr/bin/env bash
#
# Launch wrapper for cs2-server@<instance>.service. Sources the instance's env file
# (rendered by deploy-remote.sh) and builds the CS2 argv, omitting GSLT/RCON when
# unset so an instance with no token runs LAN-only instead of failing.
#
# Installed by deploy/provision/05-systemd.sh to /home/steam/cs2/cs2-launch.sh.

set -euo pipefail

Instance="${1:?usage: cs2-launch.sh <instance>}"
Root="/home/steam/cs2"
EnvFile="$Root/instances/$Instance.env"

[[ -f "$EnvFile" ]] || { echo "missing instance env: $EnvFile" >&2; exit 1; }
set -a; source "$EnvFile"; set +a

args=(-dedicated -console -usercon +map "${MAP:-de_dust2}" -port "${PORT:-27015}" +game_mode 0)
[[ -n "${GSLT:-}" ]] && args+=(+sv_setsteamaccount "$GSLT")
[[ -n "${RCON:-}" ]] && args+=(+rcon_password "$RCON")

exec "$Root/game/bin/linuxsteamrt64/cs2" "${args[@]}"
