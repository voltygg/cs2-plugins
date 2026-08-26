---
name: build-local
description: Build the CS2 plugins locally on Windows and install them into a local CS2 server. Use for "build", "compile", "does it compile", "rebuild", "run the tests", or "deploy to my local server".
---

# Build locally (cs2-plugins)

Builds this repo with CMake presets + Conan, optionally against an editable
`voltmod` checkout, and installs the result into a local CS2 dedicated server.

## When to use

The user says "build", "rebuild", "does this compile", "run the tests", or asks
to try a change on their local server. Also run this before claiming any C++
change is verified.

## 1. Use PowerShell inside the VS Dev Shell

The Bash tool's shell is **not** an x64 Native Tools shell: `cl.exe` is not on
PATH and the MSVC environment is unset. Every build command goes through the
PowerShell tool with the dev shell loaded first:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -HostArch amd64 -SkipAutomaticLocation | Out-Null
Set-Location c:\Users\admin\source\repos\m9snoi\cs2-plugins
uv run poe build
```

Shell state does not persist between PowerShell tool calls, so re-run the
launcher line in every call that compiles. A `'vswhere.exe' is not recognized`
warning from the launcher is benign — `cl` still lands on PATH.

## 2. Check for an editable voltmod first

```bash
conan editable list
```

If `voltmod` is listed, `uv run poe build` at the repo root **does not compile
it**. It only rebuilds plugin sources and links them against whatever
`vendor/voltmod/build/<preset>/voltmod-runtime.lib` already exists — the build
prints "Build Complete" and runs the plugin tests while the framework changes
are missing from the DLL. Build the framework first:

```powershell
Set-Location vendor\voltmod
uv run poe build windows-msvc-release -o "voltmod/*:with_postgres=True"
Set-Location ..\..
uv run poe build
```

`with_postgres=True` is required; without it the plugin configure step fails
with `Library 'voltmod-database' not found in package`.

To confirm the framework really rebuilt, check that
`vendor/voltmod/build/windows-msvc-release/voltmod-runtime.lib` is newer than
the edited source. Do not trust the DLL timestamp: editing only voltmod
*headers* recompiles the plugins and relinks the DLL, which looks fresh either
way.

Drop back to the published package when the coordinated work is done:

```bash
conan editable remove voltmod
```

## 3. Presets

| Preset | Output | Notes |
| --- | --- | --- |
| `windows-msvc-release` | `build/windows-msvc-release/plugins/<name>/windows-x86_64/<name>.dll` | The default. Use this to verify changes. |
| `windows-msvc-debug` | — | **Does not link.** The vendored prebuilt `libprotobuf.lib` is Release (`_ITERATOR_DEBUG_LEVEL=0`) and clashes with Debug objects, so it dies on LNK2038. It also triggers a very slow one-time Conan source build of Debug OpenSSL/libpq. |
| `linux-steamrt-release` | `.so` for the server | Builds only in the CI toolchain container, not on this machine. Do not run `uv run poe build-linux` here. |

`uv run poe build` runs Conan install, CMake configure, and compile for the
preset. **`build` never runs tests** — that is `poe test`, which recompiles
first so it cannot pass on a stale binary:

```powershell
uv run poe test
```

Tests alone, after a build:

```powershell
ctest --preset windows-msvc-release
```

Always run `poe test` (or ctest) before reporting a C++ change as verified.
`poe test -R "<regex>"` narrows it to matching cases.

## 4. Install into the local CS2 server

Requires `.env` (copy from `.env.example`) with `CS2_SERVER_PATH` set.

```powershell
uv run poe build --install admin-system           # build, then install
uv run poe build --install admin-system
uv run poe test    # ...and run CTest first
uv run poe build --install admin-system --start   # ...and launch the server
uv run poe start-server                           # launch separately
uv run poe deploy                                 # install every built plugin
```

`--install` uses the CMake component for the named plugin and merges its
server-ready `addons/` tree into `game/csgo`. It seeds `configs/settings.jsonc`
once and preserves later operator edits. Verify the load with `meta list` on the
server console — see the `rcon-debug` skill.

If a local CS2 server is running it holds the plugin DLL open and the install
fails with `[WinError 32] The process cannot access the file`. Stop the server
(or the `cs2.exe` processes) and re-run.

## Common failures

**`Library 'voltmod-database' not found in package`** — the editable voltmod was
not built with `-o "voltmod/*:with_postgres=True"`. See step 2.

**Missing SDK binaries in the Conan cache** — rebuild them from the voltmod
checkout, in the dev shell:

```powershell
Set-Location vendor\voltmod
uv run voltmod package build sdk
```

They are excluded from `--build=missing`, and the `volty` remote only holds
Linux `hl2sdk` binaries.

**`CS2Kit::HL2SDK` target names, or "nasm not in lockfile"** — the `conan.lock`
pins recipe revisions the current HEAD recipes no longer produce. Re-pin:

```powershell
conan lock remove --requires=<pkg>
conan lock create . --lockfile=conan.lock --lockfile-out=conan.lock --profile:all <windows-msvc.txt> -s build_type=Release -s compiler.runtime_type=Release
```

**Conan profiles or the remote are missing** — `uv run poe bootstrap` installs
the canonical profiles and the `volty` remote, then does a full release build.

**A local build cannot catch mis-cased includes.** Windows/MSVC resolves
includes case-insensitively; the `linux-steamrt-release` CI does not. hl2sdk has
capitalized public headers — `Color.h`, `KeyValues.h`, `CommandBuffer.h`,
`PlayerState.h` — that must be included with exact case. Review new `#include`
lines by hand; only CI will fail on this.

## Report

State the preset built, whether the editable voltmod was rebuilt first, the
CTest result, and the output path. If only a subset built or tests were skipped,
say so explicitly.
