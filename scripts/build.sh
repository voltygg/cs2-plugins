#!/usr/bin/env bash
set -euo pipefail

ScriptDir="$(cd "$(dirname "$0")" && pwd)"
cd "$ScriptDir/.."
RepoRoot="$(pwd)"

source "$ScriptDir/lib/common.sh"

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

ProfileDir="$RepoRoot/build/conan-profiles"
Profile="$ProfileDir/$Preset"
mkdir -p "$ProfileDir"

write_linux_profile() {
    # gcc-14 is the supported sniper toolchain for C++23 (std::format) with the
    # libstdc++ ABI Valve's prebuilt SDK libs use. clang is honored if CC points
    # at it. libstdc++ (not libstdc++11) matches _GLIBCXX_USE_CXX11_ABI=0.
    local cc_bin="${CC:-gcc-14}"
    local cxx_bin="${CXX:-g++-14}"
    local compiler compiler_version

    if "$cxx_bin" --version 2>/dev/null | grep -qi clang; then
        compiler="clang"
        compiler_version="$("$cxx_bin" --version | sed -n 's/.*version \([0-9][0-9]*\).*/\1/p' | head -n 1)"
        [[ -n "$compiler_version" ]] || compiler_version="17"
    else
        compiler="gcc"
        compiler_version="$("$cxx_bin" -dumpversion | cut -d. -f1)"
        [[ -n "$compiler_version" ]] || compiler_version="14"
    fi

    cat > "$Profile" <<EOF
[settings]
os=Linux
arch=x86_64
compiler=$compiler
compiler.version=$compiler_version
compiler.libcxx=libstdc++
compiler.cppstd=23
build_type=$BuildType

[conf]
tools.cmake.cmaketoolchain:generator=Ninja
tools.build:compiler_executables={"c": "$cc_bin", "cpp": "$cxx_bin"}
EOF
}

write_windows_profile() {
    local msvc_version="193"
    if command -v cl >/dev/null 2>&1; then
        local cl_version
        cl_version="$(cl 2>&1 | sed -n 's/.*Version 19\.\([0-9][0-9]\).*/\1/p' | head -n 1)"
        if [[ -n "$cl_version" ]]; then
            msvc_version="19${cl_version:0:1}"
        fi
    fi

    local runtime_type="$BuildType"
    cat > "$Profile" <<EOF
[settings]
os=Windows
arch=x86_64
compiler=msvc
compiler.version=$msvc_version
compiler.runtime=static
compiler.runtime_type=$runtime_type
compiler.cppstd=23
build_type=$BuildType

[conf]
tools.cmake.cmaketoolchain:generator=Ninja
EOF
}

case "$Preset" in
    linux-*) write_linux_profile ;;
    windows-*) write_windows_profile ;;
    *) echo "Unknown preset: $Preset" >&2; exit 1 ;;
esac

BuildDir="$RepoRoot/build/$Preset"

run_tool conan install "$RepoRoot" \
    --output-folder "$BuildDir/generators" \
    --build=missing \
    --profile:host "$Profile" \
    --profile:build "$Profile"

run_tool cmake --preset "$Preset"
run_tool cmake --build --preset "$Preset"
