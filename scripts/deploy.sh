#!/usr/bin/env bash
#
# Generic deployment script for Metamod:Source 2.0 plugins.
# Auto-detects plugin name and deploys built plugin files, configurations,
# and assets to a CS2 server.
#
# Usage:
#   deploy.sh [--server-path PATH] [--plugin-name NAME] [--architecture ARCH]
#
# Defaults:
#   --server-path   C:/cs2-server
#   --plugin-name   (auto-detected from AMBuildScript)
#   --architecture  x86_64

set -euo pipefail

ServerPath="C:/cs2-server"
PluginName=""
Architecture="x86_64"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --server-path)   ServerPath="$2"; shift 2 ;;
        --plugin-name)   PluginName="$2"; shift 2 ;;
        --architecture)  Architecture="$2"; shift 2 ;;
        -h|--help)
            sed -n '2,12p' "$0" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

# =============================================================================
# ENSURE RUNNING FROM PROJECT ROOT
# =============================================================================

ScriptDir="$(cd "$(dirname "$0")" && pwd)"
ChangedDirectory=0
if [[ "$(basename "$ScriptDir")" == "scripts" ]]; then
    pushd "$(dirname "$ScriptDir")" >/dev/null
    ChangedDirectory=1
fi

cleanup() {
    if [[ "$ChangedDirectory" -eq 1 ]]; then
        popd >/dev/null || true
    fi
}
trap cleanup EXIT

# =============================================================================
# AUTO-DETECT PLUGIN NAME
# =============================================================================

get_plugin_name() {
    if [[ -n "$PluginName" ]]; then
        echo "$PluginName"
        return
    fi

    if [[ -f "AMBuildScript" ]]; then
        local name
        name=$(grep -oE 'PLUGIN_NAME\s*=\s*"[^"]+"' AMBuildScript | head -n1 | sed -E 's/.*"([^"]+)".*/\1/')
        if [[ -n "$name" ]]; then
            echo "$name"
            return
        fi
    fi

    local dirName
    dirName="$(basename "$(pwd)")"
    echo "Warning: Could not auto-detect plugin name, using directory name: $dirName" >&2
    echo "$dirName"
}

DetectedPluginName="$(get_plugin_name)"

# Paths
CsgoPath="$ServerPath/game/csgo"
BuildDir="objdir/src/$DetectedPluginName/windows-$Architecture"

echo "=== Metamod:Source Plugin Deployment ==="
echo
echo "Plugin: $DetectedPluginName"

# Check for built binary
BinaryPath="$BuildDir/$DetectedPluginName.dll"
if [[ ! -f "$BinaryPath" ]]; then
    echo "ERROR: No built binary found at $BinaryPath" >&2
    echo "Please run the build first:" >&2
    echo "  docker compose run --rm build" >&2
    echo "  OR scripts/build.sh" >&2
    exit 1
fi

echo "Architecture: windows-$Architecture"
echo "Server Path: $ServerPath"
echo "CSGO Path: $CsgoPath"
echo

# Validate server path
if [[ ! -d "$CsgoPath" ]]; then
    echo "ERROR: CS2 server not found at $CsgoPath" >&2
    echo "Please specify the correct server path with --server-path" >&2
    exit 1
fi

# =============================================================================
# CREATE DEPLOYMENT DIRECTORIES
# =============================================================================

echo "Creating directories..."
mkdir -p "$CsgoPath/addons/metamod"

PluginAddonPath="$CsgoPath/addons/$DetectedPluginName"

if [[ -d "configs" ]]; then
    mkdir -p "$PluginAddonPath/configs"
fi

# =============================================================================
# COPY PLUGIN BINARY AND DEBUG SYMBOLS
# =============================================================================

echo "Copying plugin binary..."

BinPath="$PluginAddonPath/bin/win64"
mkdir -p "$BinPath"

cp -f "$BinaryPath" "$BinPath/$DetectedPluginName.dll"
echo "  -> addons/$DetectedPluginName/bin/win64/$DetectedPluginName.dll"

PdbPath="$BuildDir/$DetectedPluginName.pdb"
if [[ -f "$PdbPath" ]]; then
    echo "Copying debug symbols..."
    cp -f "$PdbPath" "$BinPath/$DetectedPluginName.pdb"
    echo "  -> addons/$DetectedPluginName/bin/win64/$DetectedPluginName.pdb"
fi

# =============================================================================
# COPY VCPKG RUNTIME DEPENDENCIES
# =============================================================================

VcpkgBinPath="$(pwd)/vcpkg_installed/x64-windows/bin"
if [[ -d "$VcpkgBinPath" ]]; then
    echo "Copying vcpkg runtime dependencies..."
    RuntimeDlls=(
        "pqxx.dll"
        "libpq.dll"
        "libssl-3-x64.dll"
        "libcrypto-3-x64.dll"
        "zlib1.dll"
        "lz4.dll"
    )
    for dll in "${RuntimeDlls[@]}"; do
        dllSrc="$VcpkgBinPath/$dll"
        if [[ -f "$dllSrc" ]]; then
            cp -f "$dllSrc" "$BinPath/$dll"
            echo "  -> addons/$DetectedPluginName/bin/win64/$dll"
        fi
    done
fi

# =============================================================================
# COPY VDF PLUGIN REGISTRATION
# =============================================================================

VdfPath="$DetectedPluginName.vdf"
if [[ -f "$VdfPath" ]]; then
    echo "Copying plugin registration..."
    cp -f "$VdfPath" "$CsgoPath/addons/metamod/$DetectedPluginName.vdf"
    echo "  -> addons/metamod/$DetectedPluginName.vdf"
else
    echo "Warning: $VdfPath not found (skipping)"
fi

# =============================================================================
# COPY CONFIGURATION FILES
# =============================================================================

if [[ -d "configs" ]]; then
    echo "Copying configuration files..."

    # Top-level config files
    for configFile in configs/*; do
        [[ -f "$configFile" ]] || continue
        name="$(basename "$configFile")"
        destPath="$PluginAddonPath/configs/$name"

        # Skip settings.jsonc if it already exists (preserves DB creds + server config)
        if [[ "$name" == "settings.jsonc" && -f "$destPath" ]]; then
            echo "  -> configs/$name (skipped - already exists)"
            continue
        fi

        cp -f "$configFile" "$destPath"
        echo "  -> configs/$name"
    done

    # Subdirectories (translations, etc.)
    for dir in configs/*/; do
        [[ -d "$dir" ]] || continue
        name="$(basename "$dir")"
        destDir="$PluginAddonPath/configs/$name"
        mkdir -p "$destDir"
        cp -Rf "$dir"* "$destDir/"
        echo "  -> configs/$name/"
    done
fi

# =============================================================================
# COPY GAMEDATA (SIGNATURES/OFFSETS)
# =============================================================================

CS2KitGamedata="vendor/cs2-kit/gamedata"
if [[ -d "$CS2KitGamedata" ]] && compgen -G "$CS2KitGamedata/*" >/dev/null; then
    echo "Copying CS2Kit gamedata..."
    CS2KitAddonPath="$CsgoPath/addons/cs2-kit/gamedata"
    mkdir -p "$CS2KitAddonPath"
    cp -Rf "$CS2KitGamedata"/* "$CS2KitAddonPath/" 2>/dev/null || true
    echo "  -> addons/cs2-kit/gamedata/"
fi

# =============================================================================
# DEPLOYMENT SUMMARY
# =============================================================================

echo
echo "=== Deployment Complete ==="
echo
echo "Files deployed to:"
echo "  Plugin:  $CsgoPath/addons/$DetectedPluginName/bin/win64/$DetectedPluginName.dll"
echo "  VDF:     $CsgoPath/addons/metamod/$DetectedPluginName.vdf"
if [[ -d "$PluginAddonPath/configs" ]]; then
    echo "  Configs: $CsgoPath/addons/$DetectedPluginName/configs/"
fi
echo
echo "To verify installation, run on server console: meta list"
echo
