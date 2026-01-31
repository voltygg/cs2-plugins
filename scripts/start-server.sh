#!/usr/bin/env bash

# CS2 Dedicated Server Startup Script
#
# SYNOPSIS
#     Starts the CS2 dedicated server.
#
# DESCRIPTION
#     Launches CS2 dedicated server with common settings for development/testing.
#     Optionally includes GSLT token for public server visibility.
#
# PARAMETERS
#     SERVER_PATH     Path to CS2 dedicated server root folder (default: ~/cs2-server)
#     MAP             Initial map to load (default: de_dust2)
#     GSLT_TOKEN      Game Server Login Token for public servers
#     MAX_PLAYERS     Maximum number of players (default: 64)
#     PORT            Server port (default: 27015)
#     RCON_PASSWORD   RCON password for remote administration
#
# EXAMPLES
#     ./start-server.sh
#     ./start-server.sh --map ze_example --max-players 32
#     ./start-server.sh --gslt-token "YOUR_TOKEN_HERE"

set -euo pipefail

# Default values
SERVER_PATH="${HOME}/cs2-server"
MAP="de_dust2"
GSLT_TOKEN=""
MAX_PLAYERS=64
PORT=27015
RCON_PASSWORD=""

# Color codes
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
GRAY='\033[0;90m'
NC='\033[0m' # No Color

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --server-path)
            SERVER_PATH="$2"
            shift 2
            ;;
        --map)
            MAP="$2"
            shift 2
            ;;
        --gslt-token)
            GSLT_TOKEN="$2"
            shift 2
            ;;
        --max-players)
            MAX_PLAYERS="$2"
            shift 2
            ;;
        --port)
            PORT="$2"
            shift 2
            ;;
        --rcon-password)
            RCON_PASSWORD="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --server-path PATH      CS2 server root folder (default: ~/cs2-server)"
            echo "  --map MAP              Initial map (default: de_dust2)"
            echo "  --gslt-token TOKEN     Game Server Login Token for public servers"
            echo "  --max-players N        Max players (default: 64)"
            echo "  --port N               Server port (default: 27015)"
            echo "  --rcon-password PASS   RCON password for remote admin"
            echo "  -h, --help             Show this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}ERROR: Unknown parameter: $1${NC}"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Validate server path
CS2_EXE="${SERVER_PATH}/game/bin/linuxsteamrt64/cs2"
if [[ ! -f "$CS2_EXE" ]]; then
    echo -e "${RED}ERROR: CS2 executable not found: $CS2_EXE${NC}"
    echo -e "${YELLOW}Please specify a valid server path with --server-path parameter${NC}"
    exit 1
fi

echo -e "${CYAN}=== Starting CS2 Dedicated Server ===${NC}"
echo ""
echo -e "${GRAY}Server Path: $SERVER_PATH${NC}"
echo -e "${GRAY}Map: $MAP${NC}"
echo -e "${GRAY}Max Players: $MAX_PLAYERS${NC}"
echo -e "${GRAY}Port: $PORT${NC}"
if [[ -n "$GSLT_TOKEN" ]]; then
    echo -e "${GREEN}GSLT: Enabled (public server)${NC}"
else
    echo -e "${YELLOW}GSLT: Not set (LAN mode)${NC}"
fi
echo ""

# Build command line arguments
ARGS=(
    "-dedicated"
    "-console"
    "-usercon"
    "+map" "$MAP"
    "-maxplayers" "$MAX_PLAYERS"
    "-port" "$PORT"
    "+game_mode" "0"
)

if [[ -n "$GSLT_TOKEN" ]]; then
    ARGS+=("+sv_setsteamaccount" "$GSLT_TOKEN")
fi

if [[ -n "$RCON_PASSWORD" ]]; then
    ARGS+=("+rcon_password" "$RCON_PASSWORD")
fi

echo -e "${GRAY}Command: cs2 ${ARGS[*]}${NC}"
echo ""
echo -e "${YELLOW}Starting server...${NC}"
echo -e "${GRAY}Press Ctrl+C to stop the server${NC}"
echo ""

# Start the server
cd "${SERVER_PATH}/game/bin/linuxsteamrt64" || exit 1
exec "$CS2_EXE" "${ARGS[@]}"
