#!/usr/bin/env bash
#
# Stage one built plugin into a deploy bundle under package/<name>/ (CI artifact;
# folded into Compose deploys by render.py). The layout is owned by the install()
# rules in cmake/CS2Plugin.cmake; this just runs `cmake --install` for the plugin's
# component. settings.jsonc is excluded (rendered per-server by render.py).
#
# Usage:
#   package-plugin.sh <plugin-name> [linux|windows] [--out DIR]
#
# Defaults: platform=linux, out=package/<plugin-name>

set -euo pipefail

ScriptDir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$ScriptDir/lib/common.sh"
source "$RepoRoot/scripts/lib/common.sh"  # run_tool (resolves cmake via venv/PATH)

PluginName="${1:-}"
shift || true
if [[ -z "$PluginName" || "$PluginName" == -* ]]; then
    die "Usage: package-plugin.sh <plugin-name> [linux|windows] [--out DIR]"
fi

Platform="linux"
OutDir=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        linux|windows) Platform="$1"; shift ;;
        --out) OutDir="$2"; shift 2 ;;
        *) die "unknown argument: $1" ;;
    esac
done

cd "$RepoRoot"

case "$Platform" in
    linux)   BuildPreset="${CS2_BUILD_PRESET:-linux-steamrt-release}" ;;
    windows) BuildPreset="${CS2_BUILD_PRESET:-windows-msvc-release}" ;;
    *) die "unknown platform '$Platform' (use linux|windows)" ;;
esac

PluginDir="plugins/$PluginName"
[[ -d "$PluginDir" ]] || die "plugin '$PluginDir' not found"

BuildDir="build/$BuildPreset"
if [[ ! -d "$BuildDir" ]]; then
    echo "ERROR: no build at $BuildDir" >&2
    echo "Build first (Linux): docker compose -f deploy/docker-compose.build.yml run --rm --build build" >&2
    echo "Or set CS2_BUILD_PRESET to the preset that produced the binary." >&2
    exit 1
fi

[[ -n "$OutDir" ]] || OutDir="package/$PluginName"

# Refuse an --out that resolves outside the repo - guards the rm -rf below.
RepoRootAbs="$(realpath -m -- "$RepoRoot")"
OutDirAbs="$(realpath -m -- "$OutDir")"
case "$OutDirAbs" in
    "$RepoRootAbs"/?*) : ;;  # a non-empty path *under* the repo root
    *) die "refusing unsafe output dir: $OutDir (must resolve under $RepoRootAbs)" ;;
esac
OutDir="$OutDirAbs"

echo "=== Packaging $PluginName ($Platform) -> $OutDir ==="

if [[ -e "$OutDir" && ! -d "$OutDir" ]]; then
    die "output path exists and is not a directory: $OutDir"
fi
rm -rf -- "$OutDir"

run_tool cmake --install "$BuildDir" --component "$PluginName" --prefix "$OutDir"

echo "=== Bundle ready: $OutDir ==="
