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
    local clang_bin="${CC:-clang}"
    local clangxx_bin="${CXX:-clang++}"
    local clang_version
    clang_version="$("$clang_bin" --version | sed -n 's/.*version \([0-9][0-9]*\).*/\1/p' | head -n 1)"
    [[ -n "$clang_version" ]] || clang_version="17"

    cat > "$Profile" <<EOF
[settings]
os=Linux
arch=x86_64
compiler=clang
compiler.version=$clang_version
compiler.libcxx=libstdc++
compiler.cppstd=23
build_type=$BuildType

[conf]
tools.cmake.cmaketoolchain:generator=Ninja
tools.build:compiler_executables={"c": "$clang_bin", "cpp": "$clangxx_bin"}
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
