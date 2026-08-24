# Local development

For the complete first-plugin walkthrough, start with
[Create your first plugin](getting-started-plugin.md).

## Toolchain

VoltMod pins CMake 4.3.4+, Conan 2.29.1+, Ninja, and clang-format through its
Python package. Install Python 3.14+ and uv, then run:

```powershell
uv sync
uv run poe doctor
```

Windows builds use MSVC. Linux builds use the Steam Runtime profiles.
AMBuild and vcpkg are not part of this build.

Normal builds use the published VoltMod, HL2SDK, and Metamod:Source Conan
packages. Do not initialize or add a VoltMod submodule to build plugins. The
optional `vendor/voltmod` checkout is only for coordinated framework work with
`conan editable`.

## Build presets

| Preset | Output |
| --- | --- |
| `windows-msvc-release` | Windows release DLL |
| `windows-msvc-debug` | Windows debug DLL |
| `linux-steamrt-release` | Linux Steam Runtime SO |
| `linux-steamrt-debug` | Linux Steam Runtime debug SO |

```powershell
uv run poe bootstrap
uv run poe build windows-msvc-debug
uv run poe build-linux
```

Bootstrap performs the first release build and runs tests. Later build commands
run Conan install, CMake configure, compilation, and CTest for the selected
preset.

Output is written under:

```text
build/<preset>/plugins/<plugin>/<platform-arch>/
```

## Local plugin loop

Copy [`.env.example`](../.env.example) to `.env` and set
`CS2_SERVER_PATH`. Then run:

```powershell
uv run poe dev admin-system
uv run poe start-server
```

Add `--start` to `dev` to launch immediately. Add `--no-test` for a fast local
iteration after the full suite has passed.

The installer uses the CMake component for the selected plugin and merges its
server-ready `addons/` tree into `game/csgo`. It seeds
`configs/settings.jsonc` once and preserves later operator edits.

## Common failures

### Compiler not found

Install the Visual Studio C++ workload on Windows. On Linux, select GCC 14 for
the Steam Runtime profile:

```bash
CC=gcc-14 CXX=g++-14 uv run poe build-linux
```

### Conan profiles or remote missing

```powershell
uv run poe bootstrap
```

This installs the canonical profiles and `volty` remote. Set
`VOLTMOD_SKIP_REMOTE_SETUP=1` only when another process manages Conan remotes.

### Missing HL2SDK or Metamod package

Linux builds do not build SDK packages locally. A missing binary for a
canonical profile is a package-publication problem. Check that the build uses
`linux-steamrt.txt` or `windows-msvc.txt`.

### Missing generated protobuf headers

The `hl2sdk-cs2` package supplies generated protobuf sources. Consumer builds
do not run `protoc`.
