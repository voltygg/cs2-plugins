#!/usr/bin/env bash
#
# One-command setup for the Admin System plugin.
# Run from an x64 Native Tools shell (Git Bash) on Windows, or any bash on Linux.
#
#   ./scripts/bootstrap.sh
#
# Steps: fetch submodules -> Python deps (uv) -> C++ deps (vcpkg) -> build.
# The build (configure.py + ambuild) generates the SDK protobuf headers itself,
# so there is no separate protoc step here. The Docker/Linux path in
# docker-compose.yml does its own explicit proto generation.
set -euo pipefail

ScriptDir="$(cd "$(dirname "$0")" && pwd)"
RepoRoot="$(cd "$ScriptDir/.." && pwd)"
cd "$RepoRoot"

echo "==> [1/4] Fetching submodules (cs2-kit + nested SDKs)"
git submodule update --init --recursive --depth 1

echo "==> [2/4] Installing Python build deps (AMBuild) via uv"
uv sync

echo "==> [3/4] Installing C++ deps (libpqxx, libcurl, nlohmann-json) via vcpkg"
# Static triplet (matches /MT and the AMBuildScripts): deps link into the DLL, nothing to ship alongside.
vcpkg install --triplet x64-windows-static

echo "==> [4/4] Building the plugin (configure + ambuild generate protobuf headers)"
"$ScriptDir/build.sh"

echo
echo "============================================================"
echo "  Bootstrap complete - the plugin built successfully."
echo "  Output: objdir/plugins/   (deploy with scripts/deploy.sh)"
echo "============================================================"
