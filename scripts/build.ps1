#!/usr/bin/env pwsh
Set-Location -Path (Split-Path -Parent $PSScriptRoot)

# Source environment variables
. .\env.ps1

# Configure with AMBuild (creates objdir folder)
pdm run python configure.py

# Build from the objdir folder
Set-Location -Path "objdir"
pdm run ambuild
