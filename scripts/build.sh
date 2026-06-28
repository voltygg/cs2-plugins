#!/usr/bin/env bash
set -euo pipefail

ScriptDir="$(cd "$(dirname "$0")" && pwd)"
cd "$ScriptDir/.."
RepoRoot="$(pwd)"

source "$ScriptDir/lib/common.sh"

# On Windows, import the MSVC environment (cl + INCLUDE/LIB) by sourcing
# vcvars64.bat. No-op if cl is already on PATH or the host isn't Windows.
ensure_msvc_env() {
    case "$(uname -s)" in MINGW*|MSYS*|CYGWIN*) ;; *) return 0 ;; esac
    command -v cl >/dev/null 2>&1 && return 0

    local vswhere="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
    [[ -x "$vswhere" ]] || { echo "ERROR: vswhere not found; install Visual Studio with C++ tools." >&2; exit 1; }

    local vsPath
    vsPath="$("$vswhere" -latest -products "*" \
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 \
        -property installationPath)"
    local vcvars="$vsPath\\VC\\Auxiliary\\Build\\vcvars64.bat"
    [[ -n "$vsPath" && -f "$(cygpath -u "$vcvars")" ]] || {
        echo "ERROR: vcvars64.bat not found; install the VC++ x64 toolset." >&2; exit 1; }

    echo "==> Loading MSVC environment ($vsPath)"
    # A temp .bat avoids Git Bash mangling the quotes around the space-containing
    # path. PATH is converted to POSIX; INCLUDE/LIB stay native (read by cl.exe).
    local tmp
    tmp="$(mktemp --suffix=.bat)"
    printf '@echo off\r\ncall "%s" >nul 2>&1\r\nset\r\n' "$vcvars" > "$tmp"
    local key value
    while IFS='=' read -r key value; do
        [[ "$key" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]] || continue
        if [[ "${key^^}" == "PATH" ]]; then
            export PATH="$(cygpath -u -p "$value")"
        else
            export "$key=$value"
        fi
    done < <(cmd //c "$(cygpath -w "$tmp")" | tr -d '\r')
    rm -f "$tmp"

    command -v cl >/dev/null 2>&1 || { echo "ERROR: cl still not on PATH after vcvars." >&2; exit 1; }
}

Preset="${1:-}"
if [[ -z "$Preset" ]]; then
    case "$(uname -s)" in
        MINGW*|MSYS*|CYGWIN*) Preset="windows-msvc-release" ;;
        *) Preset="linux-steamrt-release" ;;
    esac
fi

require_build_tools

case "$Preset" in
    *debug*) BuildType="Debug" ;;
    *) BuildType="Release" ;;
esac

# Static profiles (conan/profiles/); build_type and MSVC runtime_type are passed
# per build below, so one profile per OS serves Release+Debug.
ProfileDir="$RepoRoot/conan/profiles"
ExtraSettings=(-s "build_type=$BuildType")
case "$Preset" in
    linux-*)   Profile="$ProfileDir/linux-steamrt.txt" ;;
    windows-*)
        Profile="$ProfileDir/windows-msvc.txt"
        ExtraSettings+=(-s "compiler.runtime_type=$BuildType")
        ensure_msvc_env
        ;;
    *) echo "Unknown preset: $Preset" >&2; exit 1 ;;
esac

BuildDir="$RepoRoot/build/$Preset"

run_tool conan install "$RepoRoot" \
    --output-folder "$BuildDir/generators" \
    --build=missing \
    --lockfile "$RepoRoot/conan.lock" \
    --profile:host "$Profile" \
    --profile:build "$Profile" \
    "${ExtraSettings[@]}"

# Workflow preset chains configure -> build -> test in one invocation.
run_tool cmake --workflow --preset "$Preset"
