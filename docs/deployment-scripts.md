# Local deployment commands

These commands install built plugins into a local Windows CS2 server. For
remote Docker hosts, use the [production deployment guide](../deploy/README.md).

## Configure defaults

```powershell
Copy-Item .env.example .env
```

Set `CS2_SERVER_PATH` in `.env`. You can also set the build preset, SteamCMD
path, map, port, player limit, GSLT, and RCON password.

## Build, install, and launch

```powershell
uv run poe dev admin-system
uv run poe dev admin-system --start
uv run poe dev admin-system --no-test
```

`dev` builds the repository and installs only the named plugin. Tests run by
default. `--start` launches the server after installation, and `--no-test`
uses the faster configure-and-build path.

To keep the steps separate:

```powershell
uv run poe build
uv run poe deploy --plugin-name admin-system
uv run poe start-server
```

Command-line values override `.env`:

```powershell
uv run poe deploy --server-path D:/CS2-Server --plugin-name admin-system
uv run poe start-server --server-path D:/CS2-Server --map de_mirage
```

Set `CS2_BUILD_PRESET` to install another build:

```powershell
$env:CS2_BUILD_PRESET = "windows-msvc-debug"
uv run poe deploy --plugin-name admin-system
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
