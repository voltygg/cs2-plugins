<#
.SYNOPSIS
    Generic deployment script for Metamod:Source 2.0 plugins.

.DESCRIPTION
    Auto-detects plugin name and deploys built plugin files, configurations, and assets to CS2 server.

.PARAMETER ServerPath
    Path to your CS2 dedicated server root folder.
    Default: C:\cs2-server

.PARAMETER PluginName
    Override plugin name (auto-detected from AMBuildScript if not specified).

.PARAMETER Architecture
    Target architecture (x86_64 or x86).
    Default: x86_64

.EXAMPLE
    .\deploy.ps1
    Auto-detects plugin and deploys to default server path.

.EXAMPLE
    .\deploy.ps1 -ServerPath "D:\Games\CS2-Server"
    Deploys to a custom server path.

.EXAMPLE
    .\deploy.ps1 -PluginName "my-plugin"
    Override auto-detected plugin name.
#>

param(
    [string]$ServerPath = "C:\cs2-server",
    [string]$PluginName = "",
    [string]$Architecture = "x86_64"
)

$ErrorActionPreference = "Stop"

# =============================================================================
# ENSURE RUNNING FROM PROJECT ROOT
# =============================================================================

# If running from scripts/ directory, change to parent
$ScriptDir = Split-Path -Parent $PSCommandPath
if ((Split-Path -Leaf $ScriptDir) -eq "scripts") {
    Push-Location (Split-Path -Parent $ScriptDir)
    $script:ChangedDirectory = $true
} else {
    $script:ChangedDirectory = $false
}

# =============================================================================
# AUTO-DETECT PLUGIN NAME
# =============================================================================

function Get-PluginName {
    if ($PluginName) {
        return $PluginName
    }

    # Try to extract from AMBuildScript
    $ambuildScript = "AMBuildScript"
    if (Test-Path $ambuildScript) {
        $content = Get-Content $ambuildScript -Raw
        if ($content -match 'PLUGIN_NAME\s*=\s*"([^"]+)"') {
            return $matches[1]
        }
    }

    # Fallback: use directory name
    $dirName = Split-Path -Leaf (Get-Location)
    Write-Host "Warning: Could not auto-detect plugin name, using directory name: $dirName" -ForegroundColor Yellow
    return $dirName
}

$DetectedPluginName = Get-PluginName

# Paths
$CsgoPath = Join-Path $ServerPath "game\csgo"
$BuildDir = "objdir\src\$DetectedPluginName\windows-$Architecture"

Write-Host "=== Metamod:Source Plugin Deployment ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Plugin: $DetectedPluginName" -ForegroundColor Green

# Check for built binary
$BinaryPath = Join-Path $BuildDir "$DetectedPluginName.dll"
if (-not (Test-Path $BinaryPath)) {
    Write-Host "ERROR: No built binary found at $BinaryPath" -ForegroundColor Red
    Write-Host "Please run the build first:" -ForegroundColor Yellow
    Write-Host "  docker compose run --rm build" -ForegroundColor Gray
    Write-Host "  OR scripts\build.ps1 / scripts\build.sh" -ForegroundColor Gray
    exit 1
}

Write-Host "Architecture: windows-$Architecture"
Write-Host "Server Path: $ServerPath"
Write-Host "CSGO Path: $CsgoPath"
Write-Host ""

# Validate server path
if (-not (Test-Path $CsgoPath)) {
    Write-Host "ERROR: CS2 server not found at $CsgoPath" -ForegroundColor Red
    Write-Host "Please specify the correct server path with -ServerPath parameter" -ForegroundColor Yellow
    exit 1
}

# =============================================================================
# CREATE DEPLOYMENT DIRECTORIES
# =============================================================================

Write-Host "Creating directories..." -ForegroundColor Yellow
$null = New-Item -ItemType Directory -Force -Path (Join-Path $CsgoPath "addons\metamod")

# Plugin-specific directories (only if source folders exist)
$PluginAddonPath = Join-Path $CsgoPath "addons\$DetectedPluginName"

if (Test-Path "configs") {
    $null = New-Item -ItemType Directory -Force -Path (Join-Path $PluginAddonPath "configs")
}

if (Test-Path "assets") {
    $null = New-Item -ItemType Directory -Force -Path (Join-Path $PluginAddonPath "assets\layout")
    $null = New-Item -ItemType Directory -Force -Path (Join-Path $PluginAddonPath "assets\scripts")
    $null = New-Item -ItemType Directory -Force -Path (Join-Path $PluginAddonPath "assets\styles")
}

if (Test-Path "gamedata") {
    $null = New-Item -ItemType Directory -Force -Path (Join-Path $PluginAddonPath "gamedata")
}

# =============================================================================
# COPY PLUGIN BINARY AND DEBUG SYMBOLS
# =============================================================================

Write-Host "Copying plugin binary..." -ForegroundColor Yellow

# Create platform-specific binary directory in plugin's own folder
$BinPath = Join-Path $PluginAddonPath "bin\win64"
$null = New-Item -ItemType Directory -Force -Path $BinPath

Copy-Item $BinaryPath (Join-Path $BinPath "$DetectedPluginName.dll") -Force
Write-Host "  -> addons\$DetectedPluginName\bin\win64\$DetectedPluginName.dll" -ForegroundColor Gray

$PdbPath = Join-Path $BuildDir "$DetectedPluginName.pdb"
if (Test-Path $PdbPath) {
    Write-Host "Copying debug symbols..." -ForegroundColor Yellow
    Copy-Item $PdbPath (Join-Path $BinPath "$DetectedPluginName.pdb") -Force
    Write-Host "  -> addons\$DetectedPluginName\bin\win64\$DetectedPluginName.pdb" -ForegroundColor Gray
}

# =============================================================================
# COPY VDF PLUGIN REGISTRATION
# =============================================================================

$VdfPath = "$DetectedPluginName.vdf"
if (Test-Path $VdfPath) {
    Write-Host "Copying plugin registration..." -ForegroundColor Yellow
    Copy-Item $VdfPath (Join-Path $CsgoPath "addons\metamod\$DetectedPluginName.vdf") -Force
    Write-Host "  -> addons\metamod\$DetectedPluginName.vdf" -ForegroundColor Gray
} else {
    Write-Host "Warning: $VdfPath not found (skipping)" -ForegroundColor Yellow
}

# =============================================================================
# COPY CONFIGURATION FILES
# =============================================================================

if (Test-Path "configs") {
    Write-Host "Copying configuration files..." -ForegroundColor Yellow

    # Get all config files
    $ConfigFiles = Get-ChildItem "configs" -File

    foreach ($configFile in $ConfigFiles) {
        $destPath = Join-Path $PluginAddonPath "configs\$($configFile.Name)"

        # Skip database.json if it already exists (preserve existing config)
        if ($configFile.Name -eq "database.json" -and (Test-Path $destPath)) {
            Write-Host "  -> configs\$($configFile.Name) (skipped - already exists)" -ForegroundColor Cyan
            continue
        }

        Copy-Item $configFile.FullName $destPath -Force
        Write-Host "  -> configs\$($configFile.Name)" -ForegroundColor Gray
    }
}

# =============================================================================
# COPY ASSETS (PANORAMA UI)
# =============================================================================

if (Test-Path "assets") {
    $hasAssets = $false

    if (Test-Path "assets\layout") {
        $layoutFiles = Get-ChildItem "assets\layout" -Recurse -File -ErrorAction SilentlyContinue
        if ($layoutFiles) { $hasAssets = $true }
    }

    if ($hasAssets) {
        Write-Host "Copying assets..." -ForegroundColor Yellow

        if (Test-Path "assets\layout") {
            Copy-Item "assets\layout\*" (Join-Path $PluginAddonPath "assets\layout\") -Recurse -Force -ErrorAction SilentlyContinue
        }
        if (Test-Path "assets\scripts") {
            Copy-Item "assets\scripts\*" (Join-Path $PluginAddonPath "assets\scripts\") -Recurse -Force -ErrorAction SilentlyContinue
        }
        if (Test-Path "assets\styles") {
            Copy-Item "assets\styles\*" (Join-Path $PluginAddonPath "assets\styles\") -Recurse -Force -ErrorAction SilentlyContinue
        }

        Write-Host "  -> assets/" -ForegroundColor Gray
    }
}

# =============================================================================
# COPY GAMEDATA (SIGNATURES/OFFSETS)
# =============================================================================

if (Test-Path "gamedata") {
    $gamedataFiles = Get-ChildItem "gamedata" -File -ErrorAction SilentlyContinue
    if ($gamedataFiles) {
        Write-Host "Copying gamedata..." -ForegroundColor Yellow
        Copy-Item "gamedata\*" (Join-Path $PluginAddonPath "gamedata\") -Recurse -Force -ErrorAction SilentlyContinue
        Write-Host "  -> gamedata/" -ForegroundColor Gray
    }
}

# =============================================================================
# DEPLOYMENT SUMMARY
# =============================================================================

Write-Host ""
Write-Host "=== Deployment Complete ===" -ForegroundColor Green
Write-Host ""
Write-Host "Files deployed to:"
Write-Host "  Plugin:  $CsgoPath\addons\$DetectedPluginName\bin\win64\$DetectedPluginName.dll"
Write-Host "  VDF:     $CsgoPath\addons\metamod\$DetectedPluginName.vdf"
if (Test-Path (Join-Path $PluginAddonPath "configs")) {
    Write-Host "  Configs: $CsgoPath\addons\$DetectedPluginName\configs\"
}
if (Test-Path (Join-Path $PluginAddonPath "assets")) {
    Write-Host "  Assets:  $CsgoPath\addons\$DetectedPluginName\assets\"
}
Write-Host ""
Write-Host "To verify installation, run on server console: meta list" -ForegroundColor Gray
Write-Host ""

# =============================================================================
# CLEANUP
# =============================================================================

if ($script:ChangedDirectory) {
    Pop-Location
}
