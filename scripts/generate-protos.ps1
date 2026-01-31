#!/usr/bin/env pwsh
$ErrorActionPreference = "Stop"

# This script generates protobuf files for the HL2SDK
Set-Location -Path (Split-Path -Parent $PSScriptRoot)

# Source environment variables
. .\env.ps1

if (-not $HL2SDKCS2) {
    Write-Error "HL2SDKCS2 environment variable not set"
    exit 1
}

$Protoc = Join-Path $HL2SDKCS2 "devtools\bin\protoc.exe"
$ProtoPathCommon = Join-Path $HL2SDKCS2 "common"
$ProtoPathGoogle = Join-Path $HL2SDKCS2 "thirdparty\protobuf-3.21.8\src"
$OutputDir = Join-Path $HL2SDKCS2 "public"

if (-not (Test-Path $Protoc)) {
    Write-Error "protoc not found at $Protoc"
    exit 1
}

Write-Host "Generating protobuf files for hl2sdk-cs2..."
Write-Host "SDK path: $HL2SDKCS2"

# Generate common proto files
Write-Host "Generating common/*.proto..."
$ProtoFiles = Get-ChildItem -Path $ProtoPathCommon -Filter "*.proto"

foreach ($ProtoFile in $ProtoFiles) {
    Write-Host "  $($ProtoFile.Name)"
    & $Protoc `
        --proto_path="$ProtoPathCommon" `
        --proto_path="$ProtoPathGoogle" `
        --cpp_out="$OutputDir" `
        $ProtoFile.FullName
}

Write-Host "Done! Generated files are in $OutputDir"
