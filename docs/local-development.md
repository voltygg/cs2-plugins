# Local Development Setup for Windows

This project builds with Conan 2.29.1+ and CMake 4.3.4+ presets. AMBuild and
vcpkg are no longer part of the workflow.

## Prerequisites

- Windows 10/11 x64
- Git for Windows
- Visual Studio 2026 Build Tools with Desktop development with C++
- CMake 4.3.4+, Conan 2.29.1+, and Ninja installed globally or through `uv sync`

Option A (recommended): the pins ride the `voltmod` distribution this project
depends on, so one command installs everything:

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
git clone https://github.com/voltygg/cs2-plugins.git
cd cs2-plugins
```

There are no submodules - voltmod, the HL2SDK and Metamod are Conan packages.

## Build

The build auto-loads the MSVC environment via `vcvars64.bat`, so a plain shell
works on Windows.

```bash
uv sync
uv run poe bootstrap
```

`bootstrap` installs the kit's Conan profiles and the public remote, then builds.
After the first build:

```bash
uv run poe build windows-msvc-release
uv run poe build windows-msvc-debug
```

The build script runs `conan install`, configures CMake, and builds the
requested preset via the workflow preset. It prefers tools from the project
venv, then `uv run`, then `PATH`.

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
uv run poe deploy --server-path "C:/cs2-server" --plugin-name admin-system
```

Set `CS2_BUILD_PRESET` if you want to deploy from a non-default preset:

```bash
CS2_BUILD_PRESET=windows-msvc-debug uv run poe deploy --plugin-name admin-system
```

## Common Issues

### Conan cannot find a compiler

Run from an x64 Native Tools shell on Windows, or set `CC` and `CXX` on Linux:

```bash
CC=gcc-14 CXX=g++-14 uv run poe build-linux
```

### Cannot find HL2SDK or Metamod

The SDK packages come from the public Cloudsmith remote. Register it and the
profiles in one command:

```bash
conan config install https://github.com/voltygg/voltmod.git -sf conan
```

`uv run poe bootstrap` does this for you; `poe build` also adds the remote if it
is missing (set `VOLTMOD_SKIP_REMOTE_SETUP=1` to manage remotes yourself).

### ERROR: Missing binary: hl2sdk-cs2/...

The SDK packages are never built locally - the build passes
`--build=!hl2sdk-cs2/*`. This means the remote has no binary for your profile,
which is a publish problem, not a local one. Check that your profile matches
`linux-steamrt.txt` or `windows-msvc.txt`.

### Missing generated protobuf headers

Nothing generates protobuf locally any more: the `hl2sdk-cs2` package ships the
`.pb.h`/`.pb.cc` pre-generated, and its build module attaches them to the kit's
library via `hl2sdk_attach_generated_sources()`.
