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

## 3. Install Python with uv

### Install uv

uv is a fast Python package and project manager. It can install and manage Python itself, so a separate Python installation is optional.

```powershell
# Install uv via the standalone installer (recommended)
powershell -ExecutionPolicy ByPass -c "irm https://astral.sh/uv/install.ps1 | iex"

# Or via pip / pipx
pip install uv

# Verify installation
uv --version
```

> **Note:** `uv sync` (next step) provisions a compatible Python 3.14+ interpreter automatically if one isn't already on PATH. To pin/install it explicitly: `uv python install 3.14`.

## 4. Install Python Dependencies with uv

Navigate to the project root and install dependencies:

```powershell
cd C:\path\to\admin-system

# Install dependencies from uv.lock (creates .venv automatically)
uv sync
```

This installs AMBuild (from the AlliedModders GitHub repository) and other build dependencies defined in `pyproject.toml`, into a project-local `.venv`. Prefix commands with `uv run` to execute them inside that environment (e.g. `uv run ambuild`), or activate it with `.venv\Scripts\activate`.

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
uv sync

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
uv run python ../configure.py

# Build
uv run ambuild
```

Output files will be in `objdir/src/`.

## 7. Generate Visual Studio Solution

To debug and develop in Visual Studio:

```powershell
cd C:\path\to\admin-system
mkdir build-vs
cd build-vs

# Generate VS 2026 solution (--vs-version 19 = VS 2026)
uv run python ../configure.py --gen=vs --vs-version 19
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
├── .venv\                    # Python virtual environment (uv)
├── objdir\                   # AMBuild output
├── vcpkg_installed\          # vcpkg dependencies (libpqxx)
└── ...
```

## Common Issues

### "AMBuild not found"

Ensure uv dependencies are installed. AMBuild is fetched from GitHub:

```powershell
# Re-install dependencies (fetches AMBuild from GitHub)
uv sync

# Verify AMBuild is available
uv run ambuild --help
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

Protobuf headers are generated automatically by the Docker build (`docker compose run --rm build`). If
you build outside Docker and hit this error, run the two `protoc` commands from the `build` service in
[docker-compose.yml](../docker-compose.yml) manually — they generate the `*.pb.h` headers under
`vendor/hl2sdk-cs2/`.

## Quick Reference

| Task | Command |
|------|---------|
| Clone with submodules | `git clone --recursive <repo-url>` |
| Init submodules | `git submodule update --init --recursive` |
| Install Python deps | `uv sync` |
| Install C++ deps | `vcpkg install` |
| Build and deploy | `./scripts/build.sh` (Git Bash/WSL) |
| Generate VS solution | `uv run python configure.py --gen=vs --vs-version 19` |
| Start PostgreSQL | `docker compose up -d postgres` |
| Clean build | `rmdir /s /q objdir && mkdir objdir` |

## Next Steps

- Add yourself as an admin: edit [database/seed-admin.sql](../database/seed-admin.sql) with your
  SteamID64 and run it against your database. Admins live in the `admins` / `admin_groups` PostgreSQL
  tables, not a JSON file.
- Set up [settings.jsonc](../configs/settings.jsonc) with your database and plugin configuration
