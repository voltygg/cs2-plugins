# Local Development Setup for Windows

This project builds with Conan 2.29.1+ and CMake 4.3.4+ presets. AMBuild and
vcpkg are no longer part of the workflow.

## Prerequisites

- Windows 10/11 x64
- Git for Windows
- Visual Studio 2026 Build Tools with Desktop development with C++
- CMake 4.3.4+, Conan 2.29.1+, and Ninja installed globally or through `uv sync`

Option A: install the pinned build tools through this project:

```powershell
uv sync
```

Option B: install them globally with pipx if they are not already available:

```powershell
pipx install "conan>=2.29.1"
pipx install "cmake>=4.3.4"
pipx install ninja
```

## Clone

```powershell
git clone --recursive https://github.com/m9snoi/admin-system.git
cd admin-system
```

If you cloned without submodules:

```powershell
git submodule update --init --recursive
```

## Build

Open an x64 Native Tools Command Prompt for VS 2026, then launch Git Bash from
that shell so `cl` is on `PATH`.

```bash
./scripts/bootstrap.sh
```

After the first build:

```bash
./scripts/build.sh windows-msvc-release
./scripts/build.sh windows-msvc-debug
```

The build script writes a local Conan profile under `build/conan-profiles/`,
runs `conan install`, configures CMake, and builds the requested preset. It
uses tools from `PATH` first and falls back to `uv run --project .` when needed.

## CMake Presets

| Preset | Purpose |
| --- | --- |
| `windows-msvc-release` | Windows release plugin DLL |
| `windows-msvc-debug` | Windows debug plugin DLL |
| `linux-steamrt-release` | Linux Steam Runtime release plugin SO |
| `linux-steamrt-debug` | Linux Steam Runtime debug plugin SO |

Output files land under:

```text
build/<preset>/plugins/<plugin>/<platform-arch>/
```

For example:

```text
build/windows-msvc-release/plugins/admin-system/windows-x86_64/admin-system.dll
```

## Deploy To A Local Server

```bash
./scripts/deploy.sh --server-path "C:/cs2-server" --plugin-name admin-system
```

Set `CS2_BUILD_PRESET` if you want to deploy from a non-default preset:

```bash
CS2_BUILD_PRESET=windows-msvc-debug ./scripts/deploy.sh --plugin-name admin-system
```

## Common Issues

### Conan cannot find a compiler

Run from an x64 Native Tools shell on Windows, or set `CC` and `CXX` on Linux:

```bash
CC=clang CXX=clang++ ./scripts/build.sh linux-steamrt-release
```

### Cannot find HL2SDK or Metamod

Initialize submodules:

```bash
git submodule update --init --recursive
```

### Missing generated protobuf headers

CMake generates required protobuf files into `build/<preset>/vendor/cs2-kit/`.
Do not generate them into `vendor/cs2-kit/vendor/hl2sdk-cs2`.
