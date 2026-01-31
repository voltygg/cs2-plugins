#!/usr/bin/env bash

# Generic Metamod:Source 2.0 Plugin Deployment Script
#
# Auto-detects plugin name and deploys to CS2 server
#
# USAGE:
#     ./deploy.sh [OPTIONS] [SERVER_PATH]
#
# OPTIONS:
#     --plugin-name NAME    Override auto-detected plugin name
#     --arch ARCH          Architecture (x86_64 or x86, default: x86_64)
#     -h, --help           Show this help message
#
# PARAMETERS:
#     SERVER_PATH          Path to CS2 server root (default: ~/cs2-server)

set -euo pipefail

# =============================================================================
# ENSURE RUNNING FROM PROJECT ROOT
# =============================================================================

# If running from scripts/ directory, change to parent
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ "$(basename "$SCRIPT_DIR")" == "scripts" ]]; then
    cd "$SCRIPT_DIR/.."
fi

# =============================================================================
# COLOR CODES
# =============================================================================

RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
GRAY='\033[0;90m'
NC='\033[0m' # No Color

# =============================================================================
# DEFAULT VALUES
# =============================================================================

SERVER_PATH="${HOME}/cs2-server"
PLUGIN_NAME=""
ARCH="x86_64"

# =============================================================================
# PARSE ARGUMENTS
# =============================================================================

while [[ $# -gt 0 ]]; do
    case $1 in
        --plugin-name)
            PLUGIN_NAME="$2"
            shift 2
            ;;
        --arch)
            ARCH="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS] [SERVER_PATH]"
            echo ""
            echo "Options:"
            echo "  --plugin-name NAME    Override auto-detected plugin name"
            echo "  --arch ARCH          Architecture (x86_64 or x86, default: x86_64)"
            echo "  -h, --help           Show this help message"
            echo ""
            echo "Arguments:"
            echo "  SERVER_PATH          CS2 server root folder (default: ~/cs2-server)"
            exit 0
            ;;
        *)
            SERVER_PATH="$1"
            shift
            ;;
    esac
done

# =============================================================================
# AUTO-DETECT PLUGIN NAME
# =============================================================================

detect_plugin_name() {
    if [[ -n "$PLUGIN_NAME" ]]; then
        echo "$PLUGIN_NAME"
        return
    fi

    # Try to extract from AMBuildScript
    if [[ -f "AMBuildScript" ]]; then
        local name=$(grep -oP 'PLUGIN_NAME\s*=\s*"\K[^"]+' AMBuildScript 2>/dev/null || true)
        if [[ -n "$name" ]]; then
            echo "$name"
            return
        fi
    fi

    # Fallback: use directory name
    local dir_name=$(basename "$(pwd)")
    echo -e "${YELLOW}Warning: Could not auto-detect plugin name, using directory name: ${dir_name}${NC}" >&2
    echo "$dir_name"
}

DETECTED_PLUGIN_NAME=$(detect_plugin_name)
CSGO_PATH="${SERVER_PATH}/game/csgo"

# =============================================================================
# DETECT PLATFORM AND BINARY
# =============================================================================

LINUX_BUILD_DIR="objdir/src/${DETECTED_PLUGIN_NAME}/linux-${ARCH}"
WINDOWS_BUILD_DIR="objdir/src/${DETECTED_PLUGIN_NAME}/windows-${ARCH}"

if [[ -f "${LINUX_BUILD_DIR}/${DETECTED_PLUGIN_NAME}.so" ]]; then
    BINARY_PATH="${LINUX_BUILD_DIR}/${DETECTED_PLUGIN_NAME}.so"
    BINARY_EXT="so"
    PLATFORM="Linux"
    BUILD_DIR="${LINUX_BUILD_DIR}"
elif [[ -f "${WINDOWS_BUILD_DIR}/${DETECTED_PLUGIN_NAME}.dll" ]]; then
    BINARY_PATH="${WINDOWS_BUILD_DIR}/${DETECTED_PLUGIN_NAME}.dll"
    BINARY_EXT="dll"
    PLATFORM="Windows"
    BUILD_DIR="${WINDOWS_BUILD_DIR}"
else
    echo -e "${RED}ERROR: No built binary found!${NC}"
    echo "Expected one of:"
    echo "  ${LINUX_BUILD_DIR}/${DETECTED_PLUGIN_NAME}.so"
    echo "  ${WINDOWS_BUILD_DIR}/${DETECTED_PLUGIN_NAME}.dll"
    echo ""
    echo "Please run the build first:"
    echo "  docker compose run --rm build"
    echo "  OR scripts/build.sh"
    exit 1
fi

# =============================================================================
# DISPLAY DEPLOYMENT INFO
# =============================================================================

echo -e "${CYAN}=== Metamod:Source Plugin Deployment ===${NC}"
echo ""
echo -e "${GREEN}Plugin: ${DETECTED_PLUGIN_NAME}${NC}"
echo -e "Platform: ${PLATFORM}"
echo -e "Architecture: ${ARCH}"
echo -e "Server Path: ${SERVER_PATH}"
echo -e "CSGO Path: ${CSGO_PATH}"
echo ""

# Validate server path
if [[ ! -d "${CSGO_PATH}" ]]; then
    echo -e "${RED}ERROR: CS2 server not found at ${CSGO_PATH}${NC}"
    echo "Please specify the correct server path:"
    echo "  ./deploy.sh /path/to/cs2-server"
    exit 1
fi

# =============================================================================
# CREATE DEPLOYMENT DIRECTORIES
# =============================================================================

echo -e "${YELLOW}Creating directories...${NC}"
mkdir -p "${CSGO_PATH}/addons/metamod"

PLUGIN_ADDON_PATH="${CSGO_PATH}/addons/${DETECTED_PLUGIN_NAME}"

# Create plugin-specific directories only if source folders exist
[[ -d "configs" ]] && mkdir -p "${PLUGIN_ADDON_PATH}/configs"
[[ -d "assets" ]] && mkdir -p "${PLUGIN_ADDON_PATH}/assets"/{layout,scripts,styles}
[[ -d "gamedata" ]] && mkdir -p "${PLUGIN_ADDON_PATH}/gamedata"

# =============================================================================
# COPY PLUGIN BINARY
# =============================================================================

echo -e "${YELLOW}Copying plugin binary...${NC}"

# Create platform-specific binary directory in plugin's own folder
if [[ "${PLATFORM}" == "Linux" ]]; then
    BIN_PATH="${PLUGIN_ADDON_PATH}/bin/linuxsteamrt64"
else
    BIN_PATH="${PLUGIN_ADDON_PATH}/bin/win64"
fi

mkdir -p "${BIN_PATH}"

cp -v "${BINARY_PATH}" "${BIN_PATH}/${DETECTED_PLUGIN_NAME}.${BINARY_EXT}"

# =============================================================================
# COPY DEBUG SYMBOLS
# =============================================================================

if [[ "${PLATFORM}" == "Linux" ]] && [[ -f "${BUILD_DIR}/${DETECTED_PLUGIN_NAME}.so.dbg" ]]; then
    echo -e "${YELLOW}Copying debug symbols...${NC}"
    cp -v "${BUILD_DIR}/${DETECTED_PLUGIN_NAME}.so.dbg" "${BIN_PATH}/"
elif [[ "${PLATFORM}" == "Windows" ]] && [[ -f "${BUILD_DIR}/${DETECTED_PLUGIN_NAME}.pdb" ]]; then
    echo -e "${YELLOW}Copying debug symbols...${NC}"
    cp -v "${BUILD_DIR}/${DETECTED_PLUGIN_NAME}.pdb" "${BIN_PATH}/"
fi

# =============================================================================
# COPY VDF PLUGIN REGISTRATION
# =============================================================================

VDF_FILE="${DETECTED_PLUGIN_NAME}.vdf"
if [[ -f "${VDF_FILE}" ]]; then
    echo -e "${YELLOW}Copying plugin registration...${NC}"
    cp -v "${VDF_FILE}" "${CSGO_PATH}/addons/metamod/"
else
    echo -e "${YELLOW}Warning: ${VDF_FILE} not found (skipping)${NC}"
fi

# =============================================================================
# COPY CONFIGURATION FILES
# =============================================================================

if [[ -d "configs" ]]; then
    echo -e "${YELLOW}Copying configuration files...${NC}"

    # Iterate over all config files
    while IFS= read -r -d '' config_file; do
        filename=$(basename "$config_file")
        dest_path="${PLUGIN_ADDON_PATH}/configs/${filename}"

        # Skip database.json if it already exists (preserve existing config)
        if [[ "$filename" == "database.json" ]] && [[ -f "$dest_path" ]]; then
            echo -e "${CYAN}  -> configs/${filename} (skipped - already exists)${NC}"
            continue
        fi

        cp -v "$config_file" "$dest_path"
    done < <(find configs -maxdepth 1 -type f -print0)
fi

# =============================================================================
# COPY ASSETS (PANORAMA UI)
# =============================================================================

if [[ -d "assets" ]]; then
    HAS_ASSETS=false

    # Check if there are actual asset files
    if [[ -n "$(find assets -type f 2>/dev/null)" ]]; then
        HAS_ASSETS=true
    fi

    if [[ "$HAS_ASSETS" == "true" ]]; then
        echo -e "${YELLOW}Copying assets...${NC}"

        [[ -d "assets/layout" ]] && cp -rv assets/layout/* "${PLUGIN_ADDON_PATH}/assets/layout/" 2>/dev/null || true
        [[ -d "assets/scripts" ]] && cp -rv assets/scripts/* "${PLUGIN_ADDON_PATH}/assets/scripts/" 2>/dev/null || true
        [[ -d "assets/styles" ]] && cp -rv assets/styles/* "${PLUGIN_ADDON_PATH}/assets/styles/" 2>/dev/null || true

        echo -e "${GRAY}  -> assets/${NC}"
    fi
fi

# =============================================================================
# COPY GAMEDATA (SIGNATURES/OFFSETS)
# =============================================================================

if [[ -d "gamedata" ]] && [[ -n "$(find gamedata -type f 2>/dev/null)" ]]; then
    echo -e "${YELLOW}Copying gamedata...${NC}"
    cp -rv gamedata/* "${PLUGIN_ADDON_PATH}/gamedata/" 2>/dev/null || true
    echo -e "${GRAY}  -> gamedata/${NC}"
fi

# =============================================================================
# DEPLOYMENT SUMMARY
# =============================================================================

echo ""
echo -e "${GREEN}=== Deployment Complete ===${NC}"
echo ""
echo "Files deployed to:"
if [[ "${PLATFORM}" == "Linux" ]]; then
    echo "  Plugin:  ${CSGO_PATH}/addons/${DETECTED_PLUGIN_NAME}/bin/linuxsteamrt64/${DETECTED_PLUGIN_NAME}.${BINARY_EXT}"
else
    echo "  Plugin:  ${CSGO_PATH}/addons/${DETECTED_PLUGIN_NAME}/bin/win64/${DETECTED_PLUGIN_NAME}.${BINARY_EXT}"
fi
echo "  VDF:     ${CSGO_PATH}/addons/metamod/${DETECTED_PLUGIN_NAME}.vdf"
[[ -d "${PLUGIN_ADDON_PATH}/configs" ]] && echo "  Configs: ${CSGO_PATH}/addons/${DETECTED_PLUGIN_NAME}/configs/"
[[ -d "${PLUGIN_ADDON_PATH}/assets" ]] && echo "  Assets:  ${CSGO_PATH}/addons/${DETECTED_PLUGIN_NAME}/assets/"
echo ""
echo -e "${GRAY}To verify installation, run on server console: meta list${NC}"
echo ""
