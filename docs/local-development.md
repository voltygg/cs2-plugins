# Local development

```powershell
uv sync
Copy-Item .env.example .env      # set CS2_SERVER_PATH
uv run poe doctor
uv run poe bootstrap             # first build only
uv run poe build --install admin-system --start
```

For the first-plugin walkthrough see [Create your first plugin](getting-started-plugin.md); for the
admin-system's flags and database see [Admin system](admin-system.md).

## Toolchain

`uv sync` installs the CMake, Conan, Ninja and clang-format versions VoltMod pins, so nothing has
to be installed globally. You need Python 3.14+, uv, and Visual Studio 2022 or newer with the C++
workload. `uv run poe doctor` reports what is missing; it only reads.

Compiling needs MSVC on `PATH`, which the Bash shell does not have. Load a VS developer shell
first:

```powershell
$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath
& "$vs\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -HostArch amd64 -SkipAutomaticLocation | Out-Null
uv run poe build
```

| Preset | Output | Notes |
| --- | --- | --- |
| `windows-msvc-release` | `build/windows-msvc-release/plugins/<name>/windows-x86_64/<name>.dll` | The default |
| `windows-msvc-debug` | - | Does not link: the prebuilt `libprotobuf.lib` is Release and clashes on `_ITERATOR_DEBUG_LEVEL` |
| `linux-steamrt-release` | `.so` | CI toolchain container only; `poe build-linux` fails on Windows |

`poe build` compiles. `poe test` recompiles first and then runs CTest, so it cannot pass on a stale
binary; `poe test -R <regex>` narrows it. `poe lint` runs ruff, `voltmod modgraph`, and the
Panorama and schema checks.

Mis-cased includes compile on Windows and fail on Linux CI. hl2sdk's `Color.h`, `KeyValues.h`,
`CommandBuffer.h` and `PlayerState.h` need exact case.

## Working on the framework too

`conan.lock` pins a `voltmod` package revision, and a normal build resolves it from Conan. To
change the framework and a plugin together, register `vendor/voltmod` as the editable `voltmod`
package once - `uv run conan editable list` shows whether it already is:

```powershell
uv run conan editable add vendor/voltmod
uv run poe build            # compiles vendor/voltmod first, then the plugins; both incremental
uv run poe build --relock   # before committing
```

`--relock` builds the checkout, exports it with `conan export-pkg`, re-pins `voltmod` in
`conan.lock`, drops the editable, rebuilds the plugins, and fails if `build/<preset>/generators`
does not point at the new package. Commit the relocked `conan.lock` with the plugin change.

Until the `voltmod` CLI ref in `pyproject.toml` carries the editable-aware `build`, use the
checkout's own CLI: `uv run --project vendor/voltmod voltmod build [--relock]`. An older CLI links
an editable checkout without rebuilding it and still prints "Build complete".

The framework has its own suite: `uv run poe test` inside `vendor/voltmod`.

## The local server

```powershell
uv run poe build --install <plugin>            # build, then copy into CS2_SERVER_PATH
uv run poe build --install <plugin> --start    # ...and launch
uv run poe install [plugin]                    # install alone; no name installs everything
uv run poe start-server                        # launch alone
```

`--install` merges the host and the plugin's server-ready `addons/` tree into `game/csgo`, seeds
`configs/settings.jsonc` once and keeps later edits. A running server holds the DLL open and the
copy fails with `WinError 32`, so stop `cs2.exe` first. Install from another build with `--preset`,
or override `.env` on the command line:

```powershell
uv run poe install admin-system --preset windows-msvc-debug
uv run poe start-server --server-path D:/CS2-Server --map de_mirage
```

The installed tree is:

```text
game/csgo/addons/
  metamod/voltmod.vdf        the only Metamod manifest
  voltmod/bin/win64/voltmod.dll
  voltmod/gamedata/
  voltmod/plugins/<plugin>/
    plugin.json
    <plugin>.dll
    configs/
```

Verify with `volt list` on the server console. `meta list` shows the host, which is the one Metamod
plugin. Metamod has no `meta reload`; `meta unload` then `meta load`.

A custom Panorama UI also has to be compiled into your own client with `uv run poe panorama`
(Windows, needs the CS2 Workshop Tools; the client is found through Steam unless
`CS2_CLIENT_PATH` is set). Without it the server writes to a layout the client does not have.

`.env` holds the local defaults, and command-line arguments override them:

| Variable | Default | Purpose |
| --- | --- | --- |
| `CS2_SERVER_PATH` | `C:/cs2-server` | Dedicated-server root; the directory above `game/` |
| `CS2_CLIENT_PATH` | found via Steam | Your CS2 client, for `poe panorama` |
| `STEAMCMD_PATH` | `C:/Program Files/steamcmd/steamcmd.exe` | SteamCMD executable |
| `CS2_BUILD_PRESET` | `windows-msvc-release` | Preset used for local installation |
| `CS2_MAP` | `de_dust2` | Startup map |
| `CS2_PORT` | `27015` | Server port |
| `CS2_MAX_PLAYERS` | `16` | Local player limit |
| `GSLT_TOKEN` | empty | Game Server Login Token; empty starts LAN mode |
| `RCON_PASSWORD` | empty | Local RCON password |

## Reading logs

`poe start-server` runs `cs2.exe -dedicated -console -usercon` in the foreground, so the console
window is the log. To get a file, launch `cs2.exe` yourself with `-condebug` as well: everything,
VoltMod lines included, then lands in `<CS2_SERVER_PATH>/game/csgo/addons/metamod/console.log`.
`con_logfile` is not a command, and redirecting stdout stays empty because `-console` owns its own
window.

`volt log <plugin> <info|warn|error>` changes one plugin's log level on a running server.

## Common failures

| Symptom | Fix |
| --- | --- |
| `doctor` cannot find MSVC | Install the Visual Studio C++ workload, or start from a developer shell |
| `vswhere.exe is not recognized` | Put `C:\Program Files (x86)\Microsoft Visual Studio\Installer` on `PATH` |
| CS2 server path is invalid | Point `CS2_SERVER_PATH` at the directory above `game/`, not at `game/csgo` |
| Conan profiles or the `volty` remote are missing | `uv run poe bootstrap`. Set `VOLTMOD_SKIP_REMOTE_SETUP=1` only when something else manages remotes |
| Missing HL2SDK or Metamod package | A publication problem, not a source one. Check the build uses `windows-msvc.txt` or `linux-steamrt.txt` |
| Missing SDK binaries in the Conan cache | `uv run poe release build sdk` from `vendor/voltmod`, in the dev shell; they are excluded from `--build=missing` |
| The plugin is missing from `volt list` | Confirm the host is in `meta list`, then check `game/csgo/addons/voltmod/plugins/<name>/plugin.json` and `<name>.dll` |
