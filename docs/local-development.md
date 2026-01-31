# Local Development Setup for Windows

This guide walks you through setting up a local development environment for the Admin System plugin using Visual Studio 2026 on Windows.

## Prerequisites

- Windows 10/11 (64-bit)
- Administrator access (for some installations)
- Git for Windows

## 1. Install Visual Studio 2026

Download and install [Visual Studio 2026](https://visualstudio.microsoft.com/) with the following workloads:

- **Desktop development with C++**
  - MSVC v143 build tools (or latest)
  - Windows 11 SDK
  - C++ CMake tools for Windows
  - C++ AddressSanitizer (optional, for debugging)

After installation, ensure you can open the **x64 Native Tools Command Prompt for VS 2026** from the Start menu.

## 2. Install Python with PDM

### Install Python via Windows Installer

1. Download Python 3.14+ from [python.org](https://www.python.org/downloads/)
2. Run the installer and **check "Add Python to PATH"**
3. Verify installation:

   ```powershell
   python --version
   ```

### Install PDM (Python Package Manager)

PDM is a modern Python package manager with PEP 582 support.

```powershell
# Install PDM via pip
pip install pdm

# Or via pipx (recommended for CLI tools)
pipx install pdm

# Verify installation
pdm --version
```

## 3. Install Python Dependencies with PDM

Navigate to the project root and install dependencies:

```powershell
cd C:\path\to\admin-system

# Install dependencies (creates .venv automatically)
pdm install

# Activate the virtual environment (optional, PDM handles this)
pdm venv activate
```

This installs AMBuild (from the AlliedModders GitHub repository) and other build dependencies defined in `pyproject.toml`.

> **Note:** AMBuild is not available on PyPI. The `pyproject.toml` references it directly from GitHub via `git+https://github.com/alliedmodders/ambuild.git`.

## 4. Install vcpkg

vcpkg manages C++ library dependencies (libpqxx for PostgreSQL).

```powershell
# Clone vcpkg to a permanent location
cd C:\dev
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat

# Set environment variable (add to system environment variables for persistence)
setx VCPKG_ROOT "C:\dev\vcpkg"
```

### Install C++ Dependencies

```powershell
# Install libpqxx (PostgreSQL C++ client)
vcpkg install libpqxx:x64-windows

# Integrate with Visual Studio
vcpkg integrate install
```

## 5. Clone HL2SDK and Metamod:Source

The plugin requires HL2SDK-CS2 and Metamod:Source SDK.

```powershell
# Create SDK directory
mkdir C:\dev\alliedmodders
cd C:\dev\alliedmodders

# Clone HL2SDK for CS2
git clone https://github.com/alliedmodders/hl2sdk.git --branch cs2 hl2sdk-cs2

# Clone Metamod:Source
git clone https://github.com/alliedmodders/metamod-source.git --branch 1.12-dev mmsource-1.12
```

### Set Environment Variables

Add these to your system environment variables:

```powershell
setx HL2SDKCS2 "C:\dev\alliedmodders\hl2sdk-cs2"
setx MMSOURCE "C:\dev\alliedmodders\mmsource-1.12"
```

Or create a `env.bat` file in the project root:

```batch
@echo off
set HL2SDKCS2=C:\dev\alliedmodders\hl2sdk-cs2
set MMSOURCE=C:\dev\alliedmodders\mmsource-1.12
set VCPKG_ROOT=C:\dev\vcpkg
```

## 6. Build from Command Line

Open **x64 Native Tools Command Prompt for VS 2026** and run:

```powershell
cd C:\path\to\admin-system

# Source environment (if using env.bat)
call env.bat

# Create build directory
mkdir build
cd build

# Configure with AMBuild (using PDM)
pdm run python ../configure.py

# Build
pdm run ambuild
```

Output files will be in `build/package/`.

## 7. Generate Visual Studio Solution

To debug and develop in Visual Studio:

```powershell
cd C:\path\to\admin-system
mkdir build-vs
cd build-vs

# Generate VS 2026 solution (--vs-version 19 = VS 2026)
pdm run python ../configure.py --gen=vs --vs-version 19
```

Open the generated `.sln` file in Visual Studio 2026.

### Debugging Tips

1. Set the plugin DLL as the startup project
2. Configure debugging:
   - **Command**: Path to CS2 dedicated server (`srcds_win64.exe`)
   - **Command Arguments**: `-game csgo -console +map de_dust2`
   - **Working Directory**: CS2 server installation directory

## 8. Install PostgreSQL (Optional for Local Testing)

### Option A: Docker (Recommended)

```powershell
# Start PostgreSQL container
docker compose up -d postgres

# Database available at localhost:5433
```

### Option B: Local Installation

1. Download PostgreSQL 18 from [postgresql.org](https://www.postgresql.org/download/windows/)
2. Run installer with default settings
3. Set password for `postgres` user
4. Create database and user:

   ```sql
   CREATE USER admin_system WITH PASSWORD 'dev_password';
   CREATE DATABASE cs2_server OWNER admin_system;
   ```

## Directory Structure After Setup

```text
C:\dev\
├── vcpkg\                    # vcpkg package manager
├── alliedmodders\
│   ├── hl2sdk-cs2\          # HL2SDK for CS2
│   └── mmsource-1.12\       # Metamod:Source
└── ...

C:\path\to\admin-system\
├── .venv\                    # Python virtual environment (PDM)
├── build\                    # AMBuild output
├── build-vs\                 # Visual Studio solution
└── ...
```

## Common Issues

### "AMBuild not found"

Ensure PDM dependencies are installed. AMBuild is fetched from GitHub:

```powershell
# Re-install dependencies (fetches AMBuild from GitHub)
pdm install

# Verify AMBuild is available
pdm run ambuild --help
```

If the install fails, ensure Git is in your PATH and you have internet access.

### "Cannot find HL2SDK"

Verify environment variables are set:

```powershell
echo %HL2SDKCS2%
echo %MMSOURCE%
```

### vcpkg packages not found by VS

Run vcpkg integration:

```powershell
vcpkg integrate install
```

## Quick Reference

| Task | Command |
|------|---------|
| Install Python deps | `pdm install` |
| Build (command line) | `pdm run ambuild` |
| Generate VS solution | `pdm run python configure.py --gen=vs --vs-version 19` |
| Start PostgreSQL | `docker compose up -d postgres` |
| Clean build | `rmdir /s /q build && mkdir build` |

## Next Steps

- Configure [admins.json](../configs/admins.json) with your SteamID
- Set up [database.json](../configs/database.json) with your PostgreSQL credentials
