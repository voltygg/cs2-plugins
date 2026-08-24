# Local development on Windows

This project uses Conan 2.29.1+ and CMake 4.3.4+ presets. AMBuild and vcpkg are
not part of the build.

## Prerequisites

- Windows 10/11 x64
- Git for Windows
- Visual Studio 2026 Build Tools with Desktop development with C++
- CMake 4.3.4+, Conan 2.29.1+, and Ninja installed globally or through `uv sync`

The project's `voltmod` dependency carries the supported tool versions:

```powershell
uv sync
```

If the tools are not available through the project environment, install them
globally with pipx:

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

No submodule is required to build. VoltMod, HL2SDK, and Metamod are Conan
packages; `vendor/voltmod` is only for coordinated framework development.

## Build

The build loads the MSVC environment through `vcvars64.bat`, so it works from a
regular shell.

```bash
uv sync
uv run poe bootstrap
```

`bootstrap` installs the framework's Conan profiles and the public remote, then builds.
After the first build:

```bash
uv run poe build windows-msvc-release
uv run poe build windows-msvc-debug
```

The build task runs `conan install`, configures CMake, and builds the requested
preset through the workflow preset. It looks for tools in the project
environment, then through `uv run`, then on `PATH`.

## CMake presets

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

## Deploy to a local server

```bash
uv run poe deploy --server-path "C:/cs2-server" --plugin-name admin-system
```

Set `CS2_BUILD_PRESET` if you want to deploy from a non-default preset:

```bash
CS2_BUILD_PRESET=windows-msvc-debug uv run poe deploy --plugin-name admin-system
```

## Common issues

### Conan cannot find a compiler

On Windows, run from an x64 Native Tools shell if the automatic environment setup
does not work. On Linux, set `CC` and `CXX`:

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

### `ERROR: Missing binary: hl2sdk-cs2/...`

The SDK packages are never built locally - the build passes
`--build=!hl2sdk-cs2/*`. This means the remote has no binary for your profile,
which is a publish problem, not a local one. Check that your profile matches
`linux-steamrt.txt` or `windows-msvc.txt`.

### Missing generated protobuf headers

The `hl2sdk-cs2` package ships generated `.pb.h` and `.pb.cc` files. Its build
module attaches them to the framework with `hl2sdk_attach_generated_sources()`;
consumer builds do not run `protoc`.
