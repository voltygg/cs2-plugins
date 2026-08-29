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

`conan.lock` pins a `voltmod` package revision. To work on the framework, register
the checkout as the editable `voltmod` package once (`uv run conan editable list`
shows whether it is):

```powershell
uv run conan editable add vendor/voltmod
uv run poe build            # compiles vendor/voltmod first, then the plugins - both incremental
uv run poe build --relock   # before committing: export a package, pin conan.lock, drop the editable
```

The `voltmod` CLI here is installed from the git ref in `pyproject.toml`; until
that ref carries the editable-aware `build`, use the checkout's own:
`uv run --project vendor/voltmod voltmod build [--relock]`. An older CLI links an
editable checkout **without rebuilding it** - a stale DLL that prints "Build
complete".

`--relock` builds the checkout, exports it to the Conan cache (`conan export-pkg`),
re-pins `voltmod` in `conan.lock`, removes the editable, builds the plugins, and dies
if `build/<preset>/generators` does not point at the new package. Commit the
relocked `conan.lock` with the plugin change ("chore: relock voltmod for ...").
The framework's own suite runs with `uv run poe test` inside `vendor/voltmod`.

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
