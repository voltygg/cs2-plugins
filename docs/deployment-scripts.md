# Local deployment commands

These commands install built plugins into a local CS2 server. They come from
the framework CLI (`voltmod build`, `voltmod install`, `voltmod serve`), so they
work the same in any VoltMod project. For remote Docker hosts, use the
[production deployment guide](../deploy/README.md).

## Configure defaults

```powershell
Copy-Item .env.example .env
```

Set `CS2_SERVER_PATH` in `.env`. You can also set the build preset, SteamCMD
path, map, port, player limit, GSLT, and RCON password.

## Build, install, and launch

```powershell
uv run poe build --install admin-system
uv run poe build --install admin-system --start
uv run poe build --install admin-system
uv run poe test
```

`build` compiles the repository. `--install` then installs only the named
plugin into the local server, and `--start` launches that server afterwards.
Tests are a separate command so the edit-build-reload loop stays fast; run
`uv run poe test` before you commit.

To keep the steps separate:

```powershell
uv run poe build
uv run poe install admin-system
uv run poe start-server
```

`install` with no plugin name installs every built plugin.

Command-line values override `.env`:

```powershell
uv run poe install admin-system --server-path D:/CS2-Server
uv run poe start-server --server-path D:/CS2-Server --map de_mirage
```

Install from another build directory with `--preset`, or by setting
`CS2_BUILD_PRESET`:

```powershell
uv run poe install admin-system --preset windows-msvc-debug
```

## Installed layout

The installer stages the plugin's CMake component and merges:

```text
game/csgo/addons/
  metamod/<plugin>.vdf
  <plugin>/
    bin/win64/<plugin>.dll
    configs/
  voltmod/
    gamedata/
```

Linux packages use `bin/linuxsteamrt64/<plugin>.so`.

The installer keeps existing `settings.jsonc` files. It copies debug symbols
when they are available, skips empty config directories, and writes
platform-correct Metamod VDF files for production packages.

After starting the server, run `meta list` to verify the plugin.
