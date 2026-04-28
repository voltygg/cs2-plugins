# Local Development Setup for Windows

This guide walks you through setting up a local development environment for the Admin System plugin using Visual Studio 2026 on Windows.

## Prerequisites

- Windows 10/11 (64-bit)
- Administrator access (for some installations)
- Git for Windows

## 1. Clone the Repository

```powershell
# Clone with all submodules (CS2Kit and its nested SDK deps)
git clone --recursive https://github.com/m9snoi/admin-system.git
cd admin-system
```

If you already cloned without `--recursive`, initialize submodules:

```powershell
git submodule update --init --recursive
```

## 2. Install Visual Studio 2026

Download and install [Visual Studio 2026](https://visualstudio.microsoft.com/) with the following workloads:

- **Desktop development with C++**
  - MSVC v143 build tools (or latest)
  - Windows 11 SDK
  - C++ CMake tools for Windows
  - C++ AddressSanitizer (optional, for debugging)

After installation, ensure you can open the **x64 Native Tools Command Prompt for VS 2026** from the Start menu.

## 3. Install Python with PDM

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

## 4. Install Python Dependencies with PDM

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

## 5. Install vcpkg Dependencies

This project uses vcpkg in **manifest mode** with project-local installation. Visual Studio 2026 includes a bundled vcpkg, so no separate installation is required.

### Install C++ Dependencies

From the project root directory:

```powershell
cd C:\path\to\admin-system

# Install dependencies locally (uses vcpkg.json manifest)
vcpkg install

# Dependencies are installed to: vcpkg_installed/
```

This installs `libpqxx` to the `vcpkg_installed/` folder within the project. (nlohmann/json is provided by CS2Kit's vendor submodules.)

> **Note:** The `vcpkg_installed/` folder is gitignored. Each developer runs `vcpkg install` to get dependencies locally.

## 6. Build from Command Line

Open **x64 Native Tools Command Prompt for VS 2026** and run:

```powershell
cd C:\path\to\admin-system

# Install Python dependencies (includes AMBuild)
pdm install

# Install C++ dependencies (libpqxx)
vcpkg install

# Build and deploy (run via Git Bash or WSL)
./scripts/build.sh
```

Or build manually:

```powershell
# Create build directory
mkdir objdir
cd objdir

# Configure with AMBuild
pdm run python ../configure.py

# Build
pdm run ambuild
```

Output files will be in `objdir/src/`.

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
C:\path\to\admin-system\
├── vendor\
│   └── cs2-kit\              # CS2Kit library (only submodule)
│       ├── include\CS2Kit\   # Public headers
│       ├── src\              # Implementation
│       └── vendor\           # Nested SDK submodules
│           ├── hl2sdk-cs2\
│           ├── hl2sdk-manifests\
│           ├── mmsource-2.0\
│           └── nlohmann\
├── .venv\                    # Python virtual environment (PDM)
├── objdir\                   # AMBuild output
├── vcpkg_installed\          # vcpkg dependencies (libpqxx)
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

Ensure submodules are initialized:

```powershell
git submodule update --init --recursive
```

Verify the `vendor/cs2-kit/vendor/` folder contains `hl2sdk-cs2`, `hl2sdk-manifests`, and `mmsource-2.0`.

### vcpkg packages not found

Ensure you ran `vcpkg install` from the project root:

```powershell
cd C:\path\to\admin-system
vcpkg install
```

Verify the `vcpkg_installed/` folder exists and contains the dependencies.

### Fatal error: 'network_connection.pb.h' file not found

Run the [generate-protos.sh](../scripts/generate-protos.sh) to generate protobuf files in the HL2SDK:

```bash
./scripts/generate-protos.sh
```

## Quick Reference

| Task | Command |
|------|---------|
| Clone with submodules | `git clone --recursive <repo-url>` |
| Init submodules | `git submodule update --init --recursive` |
| Install Python deps | `pdm install` |
| Install C++ deps | `vcpkg install` |
| Build and deploy | `./scripts/build.sh` (Git Bash/WSL) |
| Generate VS solution | `pdm run python configure.py --gen=vs --vs-version 19` |
| Start PostgreSQL | `docker compose up -d postgres` |
| Clean build | `rmdir /s /q objdir && mkdir objdir` |

## Next Steps

- Configure [admins.json](../configs/admins.json) with your SteamID
- Set up [settings.json](../configs/settings.json) with your database and plugin configuration
