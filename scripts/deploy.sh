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
RepoRoot="$(dirname "$ScriptDir")"
source "$ScriptDir/lib/common.sh"  # run_tool (resolves cmake via venv/PATH)

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
    local buildDir="build/$BuildPreset"

    echo "--- $name ---"

    if [[ ! -d "$buildDir" ]]; then
        if [[ -n "$PluginName" ]]; then
            echo "ERROR: no build at $buildDir" >&2
            echo "Build first: scripts/build.sh" >&2
            return 1
        fi
        echo "  (skipped - no build at $buildDir)"
        return 0
    fi

    # Stage via the shared install() rules, then merge into the server tree.
    # settings.jsonc is seeded on first deploy and never clobbered.
    local staging="$buildDir/_deploy-staging/$name"
    rm -rf -- "$staging"
    if ! run_tool cmake --install "$buildDir" --component "$name" --prefix "$staging" >/dev/null; then
        if [[ -n "$PluginName" ]]; then
            echo "ERROR: cmake --install failed for $name (is it built?)" >&2
            return 1
        fi
        echo "  (skipped - cmake --install produced nothing for $name)"
        return 0
    fi

    cp -Rf "$staging/addons/." "$CsgoPath/addons/"
    echo "  -> addons/ (binary, vdf, configs, cs2-kit gamedata)"

    local settingsSrc="$pluginDir/configs/settings.jsonc"
    local settingsDst="$CsgoPath/addons/$name/configs/settings.jsonc"
    if [[ -f "$settingsSrc" ]]; then
        if [[ -f "$settingsDst" ]]; then
            echo "  -> configs/settings.jsonc (skipped - already exists)"
        else
            mkdir -p "$(dirname "$settingsDst")"
            cp -f "$settingsSrc" "$settingsDst"
            echo "  -> configs/settings.jsonc (seeded)"
        fi
    fi
}

for name in "${Plugins[@]}"; do
    deploy_plugin "$name"
done

echo
echo "=== Deployment Complete ==="
echo "Deployed plugins: ${Plugins[*]}"
echo
echo "To verify installation, run on server console: meta list"
echo
