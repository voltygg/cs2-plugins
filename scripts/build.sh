#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

# Initialize submodules if not already done
# git submodule update --init --recursive

# Configure with AMBuild (creates objdir folder)
pdm run python configure.py

# Build from the objdir folder
(cd objdir && pdm run ambuild)

# Run deploy script to copy built files to the output directory
"$(dirname "$0")/deploy.sh"
