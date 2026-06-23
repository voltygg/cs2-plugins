#!/usr/bin/env bash
set -euo pipefail

ScriptDir="$(cd "$(dirname "$0")" && pwd)"
cd "$ScriptDir/.."

# Initialize submodules if not already done
# git submodule update --init --recursive

# Configure with AMBuild (creates objdir folder)
uv run python configure.py

# Build from the objdir folder
(cd objdir && uv run ambuild)

# Run deploy script to copy built files to the output directory
"$ScriptDir/deploy.sh"
