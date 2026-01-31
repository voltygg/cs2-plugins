<#
.SYNOPSIS
    Starts the CS2 dedicated server.

.DESCRIPTION
    Launches CS2 dedicated server with common settings for development/testing.
    Optionally includes GSLT token for public server visibility.

.PARAMETER ServerPath
    Path to your CS2 dedicated server root folder.
    Default: C:\cs2-server

.PARAMETER Map
    Initial map to load.
    Default: de_dust2

.PARAMETER GsltToken
    Game Server Login Token for public servers.
    Get one from: https://steamcommunity.com/dev/managegameservers

.PARAMETER MaxPlayers
    Maximum number of players.
    Default: 64

.PARAMETER Port
    Server port.
    Default: 27015

.PARAMETER RconPassword
    RCON password for remote administration.
    Default: (none)

.EXAMPLE
    .\Start-Server.ps1
    Starts server with default settings (LAN mode).

.EXAMPLE
    .\Start-Server.ps1 -Map ze_example -MaxPlayers 32
    Starts with a specific map and player limit.

.EXAMPLE
    .\Start-Server.ps1 -GsltToken "YOUR_TOKEN_HERE"
    Starts a public server with GSLT authentication.
#>

param(
    [string]$ServerPath = "C:\cs2-server",
    [string]$Map = "de_dust2",
    [string]$GsltToken = "",
    [int]$MaxPlayers = 64,
    [int]$Port = 27015,
    [string]$RconPassword = ""
)

$ErrorActionPreference = "Stop"

# Validate server path
$Cs2Exe = Join-Path $ServerPath "game\bin\win64\cs2.exe"
if (-not (Test-Path $Cs2Exe)) {
    Write-Host "ERROR: CS2 executable not found: $Cs2Exe" -ForegroundColor Red
    Write-Host "Please specify a valid server path with -ServerPath parameter" -ForegroundColor Yellow
    exit 1
}

Write-Host "=== Starting CS2 Dedicated Server ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Server Path: $ServerPath" -ForegroundColor Gray
Write-Host "Map: $Map" -ForegroundColor Gray
Write-Host "Max Players: $MaxPlayers" -ForegroundColor Gray
Write-Host "Port: $Port" -ForegroundColor Gray
if ($GsltToken) {
    Write-Host "GSLT: Enabled (public server)" -ForegroundColor Green
} else {
    Write-Host "GSLT: Not set (LAN mode)" -ForegroundColor Yellow
}
Write-Host ""

# Build command line arguments
$Args = @(
    "-dedicated"
    "-console"
    "-usercon"
    "+map $Map"
    "-maxplayers $MaxPlayers"
    "-port $Port"
	"+game_mode 0"
)

if ($GsltToken) {
    $Args += "+sv_setsteamaccount $GsltToken"
}

if ($RconPassword) {
    $Args += "+rcon_password $RconPassword"
}

$ArgsString = $Args -join " "

Write-Host "Command: cs2.exe $ArgsString" -ForegroundColor DarkGray
Write-Host ""
Write-Host "Starting server..." -ForegroundColor Yellow
Write-Host "Press Ctrl+C to stop the server" -ForegroundColor Gray
Write-Host ""

# Start the server
Push-Location (Join-Path $ServerPath "game\bin\win64")
try {
    & $Cs2Exe $Args
} finally {
    Pop-Location
}
