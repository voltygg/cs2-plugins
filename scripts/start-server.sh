#!/usr/bin/env bash
#
# Starts the CS2 dedicated server.
#
# Usage:
#   start-server.sh [--server-path PATH] [--steamcmd-path PATH] [--map MAP]
#                   [--gslt-token TOKEN] [--max-players N] [--port N]
#                   [--rcon-password PASS] [--check-update]
#
# Defaults:
#   --server-path     C:/cs2-server
#   --steamcmd-path   C:/Program Files/steamcmd/steamcmd.exe
#   --map             de_dust2
#   --max-players     64
#   --port            27015

set -euo pipefail

ServerPath="C:/cs2-server"
SteamCmdPath="C:/Program Files/steamcmd/steamcmd.exe"
Map="de_dust2"
GsltToken=""
MaxPlayers=64
Port=27015
RconPassword=""
CheckUpdate=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --server-path)     ServerPath="$2"; shift 2 ;;
        --steamcmd-path)   SteamCmdPath="$2"; shift 2 ;;
        --map)             Map="$2"; shift 2 ;;
        --gslt-token)      GsltToken="$2"; shift 2 ;;
        --max-players)     MaxPlayers="$2"; shift 2 ;;
        --port)            Port="$2"; shift 2 ;;
        --rcon-password)   RconPassword="$2"; shift 2 ;;
        --check-update)    CheckUpdate=1; shift ;;
        -h|--help)
            sed -n '2,16p' "$0" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

# Update server via SteamCMD
if [[ "$CheckUpdate" -eq 1 ]]; then
    if [[ -f "$SteamCmdPath" ]]; then
        echo "=== Updating CS2 Dedicated Server ==="
        set +e
        "$SteamCmdPath" +force_install_dir "$ServerPath" +login anonymous +app_update 730 validate +quit
        rc=$?
        set -e
        if [[ $rc -ne 0 ]]; then
            echo "WARNING: SteamCMD update failed (exit code $rc). Continuing with existing files..."
        else
            echo "Server update complete."
        fi
        echo
    else
        echo "WARNING: SteamCMD not found at $SteamCmdPath. Skipping update."
        echo "Install SteamCMD or use --steamcmd-path to specify its location."
        echo
    fi
fi

# Validate server path
Cs2Exe="$ServerPath/game/bin/win64/cs2.exe"
if [[ ! -f "$Cs2Exe" ]]; then
    echo "ERROR: CS2 executable not found: $Cs2Exe" >&2
    echo "Please specify a valid server path with --server-path" >&2
    exit 1
fi

echo "=== Starting CS2 Dedicated Server ==="
echo
echo "Server Path: $ServerPath"
echo "Map: $Map"
echo "Max Players: $MaxPlayers"
echo "Port: $Port"
if [[ -n "$GsltToken" ]]; then
    echo "GSLT: Enabled (public server)"
else
    echo "GSLT: Not set (LAN mode)"
fi
echo

# Build command line arguments
Args=(
    -dedicated
    -console
    -usercon
    +map "$Map"
    -maxplayers "$MaxPlayers"
    -port "$Port"
    +game_mode 0
)

if [[ -n "$GsltToken" ]]; then
    Args+=(+sv_setsteamaccount "$GsltToken")
fi

if [[ -n "$RconPassword" ]]; then
    Args+=(+rcon_password "$RconPassword")
fi

echo "Command: cs2.exe ${Args[*]}"
echo
echo "Starting server..."
echo "Press Ctrl+C to stop the server"
echo

# Start the server
pushd "$ServerPath/game/bin/win64" >/dev/null
trap 'popd >/dev/null || true' EXIT
"$Cs2Exe" "${Args[@]}"
