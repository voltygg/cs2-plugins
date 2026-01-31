#!/bin/bash
cd "$(dirname "$0")/.."

# Source environment variables
source env.sh

# Configure with AMBuild (creates objdir folder)
pdm run python configure.py

# Build from the objdir folder
cd objdir
pdm run ambuild
