# Create your first plugin

This guide takes a Windows contributor from a clean checkout to a running
VoltMod plugin that answers `!ping`.

## Prerequisites

Install:

- Git
- [uv](https://docs.astral.sh/uv/)
- Python 3.14 or newer
- Visual Studio 2022 or newer with Desktop development with C++
- A local CS2 dedicated server
- Metamod:Source 2

You do not need global CMake, Conan, Ninja, or clang-format installations.
`uv sync` installs the versions pinned by VoltMod.

## Set up the repository

```powershell
git clone https://github.com/voltygg/cs2-plugins.git
cd cs2-plugins
Copy-Item .env.example .env
uv sync
```

Edit `.env` and set `CS2_SERVER_PATH` to the dedicated-server root. The
directory must contain `game/csgo`.

Check the environment before starting a build:

```powershell
uv run poe doctor --server-path C:/cs2-server
```

`doctor` is read-only. It reports the tool versions, compiler, project files,
Conan configuration, CS2 executable, and Metamod installation. Missing Conan
profiles or the `volty` remote are warnings before the first bootstrap because
bootstrap installs them.

## Run the first build

```powershell
uv run poe bootstrap
```

Bootstrap:

1. Installs VoltMod's Conan profiles and public package remote.
2. Selects `windows-msvc-release`.
3. Resolves VoltMod, HL2SDK, Metamod:Source, and other dependencies.
4. Configures CMake.
5. Builds every target.
6. Runs CTest.

Success ends with a build path under `build/windows-msvc-release`. Use
`uv run poe build` for later full builds.

## Scaffold a plugin

Plugin names use kebab case:

```powershell
uv run poe new-plugin hello-world
```

The command creates and registers:

```text
plugins/hello-world/
  CMakeLists.txt
  configs/
    settings.jsonc
    settings.schema.json
    translations/en.json
  src/
    App.cpp
    App.hpp
    Commands.cpp
    Config.hpp
    Plugin.cpp
    Plugin.hpp
```

You do not need to edit the root `CMakeLists.txt`. The scaffold adds its own
`add_subdirectory(plugins/hello-world)` entry.

The generated plugin already:

- derives from `VoltMod::MetamodPlugin`;
- owns one load-cycle `App`;
- loads JSONC settings and translations;
- publishes build information to `meta list`;
- registers `!ping`.

## Build and run it

```powershell
uv run poe dev hello-world --start
```

`dev` builds the configured preset, runs tests, installs only
`hello-world` into `CS2_SERVER_PATH`, preserves an existing
`settings.jsonc`, and starts the server when `--start` is present.

For a faster iteration after the full test suite has passed:

```powershell
uv run poe dev hello-world --no-test
```

To install without launching, omit `--start`. To launch later:

```powershell
uv run poe start-server
```

## Verify the result

In the server console:

```text
meta list
```

Confirm `hello-world` appears with its version and build revision. Then join
the server and enter:

```text
!ping
```

The translated pong reply confirms that the plugin loaded, command routing is
active, and its translation file was installed.

## Make a change

- Edit plugin startup and composition in `src/App.cpp`.
- Add commands in `src/Commands.cpp` or another `.cpp` below `src/`.
- Define settings in `src/Config.hpp` and update both config files.
- Add player-facing messages to every file under `configs/translations/`.
- Put SDK-free logic in plain C++ types so it can be unit-tested.

The normal loop is:

```powershell
uv run poe dev hello-world --no-test
```

Metamod plugins are native modules. Restart the local server after replacing a
loaded DLL if the operating system keeps it locked. Before pushing, run:

```powershell
uv run poe lint
uv run poe format-check
uv run poe build
```

## Local settings

`.env` supports:

| Variable | Default | Purpose |
| --- | --- | --- |
| `CS2_SERVER_PATH` | `C:/cs2-server` | Dedicated-server root |
| `STEAMCMD_PATH` | `C:/Program Files/steamcmd/steamcmd.exe` | SteamCMD executable |
| `CS2_BUILD_PRESET` | `windows-msvc-release` | Build selected for local installation |
| `CS2_MAP` | `de_dust2` | Startup map |
| `CS2_PORT` | `27015` | Server port |
| `CS2_MAX_PLAYERS` | `16` | Local player limit |
| `GSLT_TOKEN` | empty | Game Server Login Token; empty starts LAN mode |
| `RCON_PASSWORD` | empty | Optional local RCON password |

Command-line arguments override these defaults. Environment variables override
the values loaded from `.env`.

## Troubleshooting

### Doctor cannot find MSVC

Install the Visual Studio C++ workload. The build can import
`vcvars64.bat` automatically from a normal PowerShell session.

### CS2 server path is invalid

Point `CS2_SERVER_PATH` at the directory above `game/`, not at `game/csgo`.

### The plugin is missing from `meta list`

Confirm Metamod loads first, then inspect:

```text
game/csgo/addons/metamod/hello-world.vdf
game/csgo/addons/hello-world/bin/win64/hello-world.dll
```

Run `uv run poe dev hello-world` again after correcting the path.

### Conan cannot resolve SDK packages

Run `uv run poe bootstrap`. It installs the canonical profiles and public
package remote. If a published SDK binary is missing for a canonical profile,
the package publication must be fixed; changing plugin source will not resolve
it.
