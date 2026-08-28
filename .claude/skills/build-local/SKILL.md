---
name: build-local
description: Build the CS2 plugins locally on Windows and install them into a local CS2 server. Use for "build", "compile", "does it compile", "rebuild", "run the tests", or "deploy to my local server".
---

# Build locally (cs2-plugins)

Builds this repo with CMake presets + Conan, optionally against the `vendor/voltmod`
checkout, and installs the result into the local CS2 dedicated server named by
`CS2_SERVER_PATH` in `.env` (copy `.env.example`).

## When to use

The user says "build", "rebuild", "does this compile", "run the tests", or asks to
try a change on their local server. Also run this before claiming any C++ change
is verified.

## 1. Compile from a VS developer shell

The Bash tool's shell has no MSVC on PATH. Every command that compiles goes through
the PowerShell tool with the dev shell loaded first; shell state does not persist
between calls, so repeat the two launcher lines in every compiling call:

```powershell
$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath
& "$vs\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -HostArch amd64 -SkipAutomaticLocation | Out-Null
uv run poe build
```

A `'vswhere.exe' is not recognized` line printed by the launcher itself is benign.

## 2. Consuming the framework

`conan.lock` pins a `voltmod` package revision. A plain `uv run poe build` uses it
and never compiles `vendor/voltmod`, so a framework edit is invisible until a new
package exists and the lock names it:

```powershell
Set-Location vendor\voltmod
uv run voltmod package build kit
Set-Location ..\..
uv run conan lock remove --requires="voltmod/*" --lockfile=conan.lock --lockfile-out=conan.lock
uv run conan lock create . --profile:all vendor/voltmod/conan/profiles/windows-msvc.txt -s build_type=Release -s compiler.runtime_type=Release --lockfile=conan.lock --lockfile-out=conan.lock --no-remote
uv run poe build
```

`conan lock remove` first: `--update` does not re-pin a revision the lock already
names. Commit the relocked `conan.lock` with the plugin change ("chore: relock
voltmod for ...").

If `uv run conan editable list` still shows `voltmod`, remove it once with
`uv run conan editable remove voltmod`; the editable flow is gone.

**ABI change in a VoltMod header** (a member added to `Runtime` or another
by-value service): wipe `build/<preset>` before rebuilding the plugins. A package
rebuild alone relinks against stale layouts and ships a DLL that loads and then
misbehaves.

## 3. Presets and tests

| Preset | Output | Notes |
| --- | --- | --- |
| `windows-msvc-release` | `build/windows-msvc-release/plugins/<name>/windows-x86_64/<name>.dll` | The default; use it to verify changes. |
| `windows-msvc-debug` | - | Does not link: the prebuilt `libprotobuf.lib` is Release and clashes on `_ITERATOR_DEBUG_LEVEL`. |
| `linux-steamrt-release` | `.so` | CI toolchain container only. `poe build-linux` fails on Windows. |

`poe build` compiles only. `poe test` recompiles first and then runs CTest, so it
cannot pass on a stale binary; `poe test -R <regex>` narrows it. Run one of them
before reporting a C++ change as verified. The framework has its own suite:
`uv run poe test` inside `vendor/voltmod`.

`uv run poe lint` runs ruff and `voltmod modgraph`, which rejects forward
declarations in plugin headers, anonymous namespaces and using-directives.

## 4. Install into the local server

```powershell
uv run poe build --install <plugin>            # build, then copy into CS2_SERVER_PATH
uv run poe build --install <plugin> --start    # ...and launch the server
uv run poe start-server                        # launch alone
```

`--install` merges the plugin's server-ready `addons/` tree into `game/csgo`,
seeds `configs/settings.jsonc` once and preserves later edits. A running server
holds the DLL open and the copy fails with `WinError 32`: stop `cs2.exe` first.

A custom Panorama UI also needs the layouts compiled into your own client:
`uv run poe panorama` (Windows, needs the CS2 Workshop Tools; the client is found
through Steam unless `CS2_CLIENT_PATH` is set). Without it the server writes to a
layout the client does not have and shows nothing.

Verify the load with `meta list` on the server console (see `rcon-debug`).

## Common failures

- **`Library 'voltmod-database' not found in package`** - the framework package was
  built without PostgreSQL. `voltmod package build kit` includes it; a hand-run
  `poe build` in `vendor/voltmod` needs `-o "voltmod/*:with_postgres=True"`.
- **Missing SDK binaries in the Conan cache** - `uv run voltmod package build sdk`
  from `vendor/voltmod`, in the dev shell. They are excluded from `--build=missing`.
- **Lock names a recipe revision that no longer exists** (`nasm not in lockfile`,
  odd target names) - re-pin with the `conan lock remove` / `conan lock create`
  pair above for that package.
- **Profiles or the remote are missing** - `uv run poe bootstrap`.
- **Mis-cased includes** compile here and fail on Linux CI: hl2sdk's `Color.h`,
  `KeyValues.h`, `CommandBuffer.h`, `PlayerState.h` need exact case.

## Report

State the preset built, whether the framework package was rebuilt and relocked,
the CTest result, and the output path. If only a subset built or tests were
skipped, say so.
