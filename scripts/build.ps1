#!/usr/bin/env pwsh
Set-Location -Path (Split-Path -Parent $PSScriptRoot)

# Initialize submodules if not already done
# git submodule update --init --recursive

# Configure with AMBuild (creates objdir folder)
pdm run python configure.py

# Build from the objdir folder
Set-Location -Path "objdir"
pdm run ambuild

Set-Location -Path "../scripts"

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
}

# Run deploy script to copy built files to the output directory
.\deploy.ps1
