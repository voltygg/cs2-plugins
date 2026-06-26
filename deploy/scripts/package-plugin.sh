#!/usr/bin/env bash
#
# Stage one built plugin into a self-contained deploy bundle under package/<name>/,
# laid out exactly as it lands under a server's csgo/ tree. The bundle is what CI
# uploads as an artifact and what deploy/tools/render.py folds into Compose deploys.
#
# Unlike scripts/deploy.sh (local Windows dev), this targets Linux by default and
# GENERATES the platform-correct VDF (the win64/linuxsteamrt64 path segment is part
# of the VDF "file" value, so it must differ per platform). settings.jsonc is NOT
# included here - it is rendered per-server by render.py at deploy time.
#
# Usage:
#   package-plugin.sh <plugin-name> [linux|windows] [--out DIR]
#
# Defaults: platform=linux, out=package/<plugin-name>

set -euo pipefail

ScriptDir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$ScriptDir/lib/common.sh"

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
    linux)   BinSubdir="linuxsteamrt64"; ObjArch="linux-x86_64";   BinExt="so"; DbgExt="so.dbg" ;;
    windows) BinSubdir="win64";          ObjArch="windows-x86_64"; BinExt="dll"; DbgExt="pdb" ;;
    *) die "unknown platform '$Platform' (use linux|windows)" ;;
esac

PluginDir="plugins/$PluginName"
if [[ ! -d "$PluginDir" ]]; then
    die "plugin '$PluginDir' not found"
fi

BuildDir="objdir/plugins/$PluginName/src/$PluginName/$ObjArch"
BinaryPath="$BuildDir/$PluginName.$BinExt"
if [[ ! -f "$BinaryPath" ]]; then
    echo "ERROR: no built binary at $BinaryPath" >&2
    echo "Build first (Linux): docker compose -f deploy/docker-compose.build.yml run --rm --build build" >&2
    exit 1
fi

[[ -n "$OutDir" ]] || OutDir="package/$PluginName"
[[ "$OutDir" != "/" && "$OutDir" != "." ]] || die "refusing unsafe output dir: $OutDir"
AddonsDir="$OutDir/addons"
PluginAddon="$AddonsDir/$PluginName"

echo "=== Packaging $PluginName ($Platform) -> $OutDir ==="

rm -rf -- "$OutDir"
mkdir -p "$PluginAddon/bin/$BinSubdir" "$AddonsDir/metamod"

# Binary (+ debug symbols if present).
cp -f "$BinaryPath" "$PluginAddon/bin/$BinSubdir/$PluginName.$BinExt"
echo "  -> addons/$PluginName/bin/$BinSubdir/$PluginName.$BinExt"
if [[ -f "$BuildDir/$PluginName.$DbgExt" ]]; then
    cp -f "$BuildDir/$PluginName.$DbgExt" "$PluginAddon/bin/$BinSubdir/$PluginName.$DbgExt"
    echo "  -> addons/$PluginName/bin/$BinSubdir/$PluginName.$DbgExt"
fi

# Generated, platform-correct VDF.
cat > "$AddonsDir/metamod/$PluginName.vdf" <<EOF
"Metamod Plugin"
{
	"alias"	"$PluginName"
	"file"	"addons/$PluginName/bin/$BinSubdir/$PluginName"
}
EOF
echo "  -> addons/metamod/$PluginName.vdf (generated for $Platform)"

# Configs EXCEPT settings.jsonc (rendered per-server at deploy time).
if [[ -d "$PluginDir/configs" ]]; then
    mkdir -p "$PluginAddon/configs"
    for entry in "$PluginDir"/configs/*; do
        [[ -e "$entry" ]] || continue
        base="$(basename "$entry")"
        [[ "$base" == "settings.jsonc" ]] && continue
        cp -Rf "$entry" "$PluginAddon/configs/"
        echo "  -> addons/$PluginName/configs/$base"
    done
fi

# Shared cs2-kit gamedata (copied once per bundle).
Gamedata="vendor/cs2-kit/gamedata"
if [[ -d "$Gamedata" ]] && compgen -G "$Gamedata/*" >/dev/null; then
    mkdir -p "$AddonsDir/cs2-kit/gamedata"
    cp -Rf "$Gamedata"/* "$AddonsDir/cs2-kit/gamedata/"
    echo "  -> addons/cs2-kit/gamedata/"
fi

echo "=== Bundle ready: $OutDir ==="
