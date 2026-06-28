#!/usr/bin/env bash
#
# One-command setup for the Admin System plugin.
# Run from an x64 Native Tools shell (Git Bash) on Windows, or any bash on Linux.
#
#   ./scripts/bootstrap.sh
#
# Steps: fetch submodules -> check build tools -> Conan install + CMake build.
# CMake generates required SDK protobuf files into build/<preset>/, never into
# the vendored HL2SDK tree.
set -euo pipefail

ScriptDir="$(cd "$(dirname "$0")" && pwd)"
RepoRoot="$(cd "$ScriptDir/.." && pwd)"
cd "$RepoRoot"

# Fetch first: lib/common.sh wraps helpers vendored in cs2-kit.
echo "==> [1/3] Fetching submodules (cs2-kit + nested SDKs)"
git submodule update --init --recursive --depth 1

source "$ScriptDir/lib/common.sh"

echo "==> [2/3] Checking CMake + Conan build tools"
require_build_tools

echo "==> [3/3] Building with Conan + CMake"
bash "$ScriptDir/build.sh"

echo
echo "============================================================"
echo "  Bootstrap complete - the plugin built successfully."
echo "  Output: build/<preset>/plugins/"
echo "============================================================"
