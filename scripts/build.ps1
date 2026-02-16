#!/usr/bin/env pwsh
Set-Location -Path (Split-Path -Parent $PSScriptRoot)

# Source environment variables
. .\env.ps1

# Configure with AMBuild (creates objdir folder)
pdm run python configure.py

# Build from the objdir folder
Set-Location -Path "objdir"
pdm run ambuild

Set-Location -Path "../scripts"

# Run deploy script to copy built files to the output directory
.\deploy.ps1
