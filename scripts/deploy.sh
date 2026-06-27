#!/usr/bin/env bash
#
# Deployment script for the Metamod:Source 2.0 plugins in this monorepo.
# Deploys built plugin binaries, configurations, and the shared cs2-kit gamedata
# to a CS2 server.
#
# Usage:
#   deploy.sh [--server-path PATH] [--plugin-name NAME] [--architecture ARCH]
#
# Defaults:
#   --server-path   C:/cs2-server
#   --plugin-name   (all plugins under plugins/ that have a built binary)
#   --architecture  x86_64

set -euo pipefail

ServerPath="C:/cs2-server"
PluginName=""
Architecture="x86_64"
BuildPreset="${CS2_BUILD_PRESET:-windows-msvc-release}"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --server-path)   ServerPath="$2"; shift 2 ;;
        --plugin-name)   PluginName="$2"; shift 2 ;;
        --architecture)  Architecture="$2"; shift 2 ;;
        -h|--help)
            sed -n '2,13p' "$0" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

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

CsgoPath="$ServerPath/game/csgo"

if [[ "$Architecture" != "x86_64" ]]; then
    echo "ERROR: only x86_64 is supported by the CMake build." >&2
    exit 1
fi

if [[ ! -d "$CsgoPath" ]]; then
    echo "ERROR: CS2 server not found at $CsgoPath" >&2
    echo "Please specify the correct server path with --server-path" >&2
    exit 1
fi

Plugins=()
if [[ -n "$PluginName" ]]; then
    if [[ ! -d "plugins/$PluginName" ]]; then
        echo "ERROR: plugin 'plugins/$PluginName' not found" >&2
        exit 1
    fi
    Plugins+=("$PluginName")
else
    for dir in plugins/*/; do
        [[ -d "$dir" ]] || continue
        Plugins+=("$(basename "$dir")")
    done
fi

if [[ ${#Plugins[@]} -eq 0 ]]; then
    echo "ERROR: no plugins found under plugins/" >&2
    exit 1
fi

echo "=== Metamod:Source Plugin Deployment ==="
echo
echo "Server Path: $ServerPath"
echo "CSGO Path:   $CsgoPath"
echo "Architecture: windows-$Architecture"
echo "Build preset:  $BuildPreset"
echo

mkdir -p "$CsgoPath/addons/metamod"

deploy_plugin() {
    local name="$1"
    local pluginDir="plugins/$name"
    local buildDir="build/$BuildPreset/plugins/$name/windows-$Architecture"
    local binaryPath="$buildDir/$name.dll"

    echo "--- $name ---"

    if [[ ! -f "$binaryPath" ]]; then
        if [[ -n "$PluginName" ]]; then
            echo "ERROR: No built binary found at $binaryPath" >&2
            echo "Build first: scripts/build.sh" >&2
            return 1
        fi
        echo "  (skipped - no built binary at $binaryPath)"
        return 0
    fi

    local pluginAddonPath="$CsgoPath/addons/$name"
    local binPath="$pluginAddonPath/bin/win64"
    mkdir -p "$binPath"

    cp -f "$binaryPath" "$binPath/$name.dll"
    echo "  -> addons/$name/bin/win64/$name.dll"

    local pdbPath="$buildDir/$name.pdb"
    if [[ -f "$pdbPath" ]]; then
        cp -f "$pdbPath" "$binPath/$name.pdb"
        echo "  -> addons/$name/bin/win64/$name.pdb"
    fi

    local vdfPath="$pluginDir/$name.vdf"
    if [[ -f "$vdfPath" ]]; then
        cp -f "$vdfPath" "$CsgoPath/addons/metamod/$name.vdf"
        echo "  -> addons/metamod/$name.vdf"
    else
        echo "  Warning: $vdfPath not found (skipping VDF)"
    fi

    if [[ -d "$pluginDir/configs" ]]; then
        mkdir -p "$pluginAddonPath/configs"

        for configFile in "$pluginDir"/configs/*; do
            [[ -f "$configFile" ]] || continue
            local cfgName destPath
            cfgName="$(basename "$configFile")"
            destPath="$pluginAddonPath/configs/$cfgName"

            # Preserve an existing settings.jsonc (holds DB creds + server config)
            if [[ "$cfgName" == "settings.jsonc" && -f "$destPath" ]]; then
                echo "  -> configs/$cfgName (skipped - already exists)"
                continue
            fi

            cp -f "$configFile" "$destPath"
            echo "  -> configs/$cfgName"
        done

        for dir in "$pluginDir"/configs/*/; do
            [[ -d "$dir" ]] || continue
            local subName destDir
            subName="$(basename "$dir")"
            destDir="$pluginAddonPath/configs/$subName"
            mkdir -p "$destDir"
            cp -Rf "$dir"* "$destDir/"
            echo "  -> configs/$subName/"
        done
    fi
}

for name in "${Plugins[@]}"; do
    deploy_plugin "$name"
done

# Shared across all plugins — copy once.
CS2KitGamedata="vendor/cs2-kit/gamedata"
if [[ -d "$CS2KitGamedata" ]] && compgen -G "$CS2KitGamedata/*" >/dev/null; then
    echo "--- shared cs2-kit gamedata ---"
    CS2KitAddonPath="$CsgoPath/addons/cs2-kit/gamedata"
    mkdir -p "$CS2KitAddonPath"
    cp -Rf "$CS2KitGamedata"/* "$CS2KitAddonPath/" 2>/dev/null || true
    echo "  -> addons/cs2-kit/gamedata/"
fi

echo
echo "=== Deployment Complete ==="
echo "Deployed plugins: ${Plugins[*]}"
echo
echo "To verify installation, run on server console: meta list"
echo
