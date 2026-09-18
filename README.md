# CS2 plugins

Native C++23 plugins for Counter-Strike 2 dedicated servers, built on the
[VoltMod framework](https://github.com/voltygg/voltmod). VoltMod, HL2SDK and Metamod:Source come
from Conan, so a framework checkout is only needed when you change the framework and a plugin
together.

## Plugins

| Plugin | What it does | Needs |
| --- | --- | --- |
| [admin-system](plugins/admin-system/README.md) | Admins, punishments, menus and reports. | PostgreSQL, MariaDB or SQLite |
| [anticheat](plugins/anticheat/README.md) | Server-side cheat detection: aim analysis over correlated shots plus client-integrity checks. | admin-system for alerts and bans; detection runs without it |
| [bhop](plugins/bhop/README.md) | Smooth, client-predicted bunnyhop with per-player session grants. | admin-system only in grants mode |

`plugins/contracts/` holds the interfaces plugins publish to each other.

## Install a release

You need a CS2 dedicated server and
[Metamod:Source 2](https://www.sourcemm.net/downloads.php/?branch=master).

1. Download the plugin archive from the repository releases.
2. Extract it into the server's `game/csgo` directory. It contains the voltmod host and the plugin.
3. Edit `game/csgo/addons/voltmod/plugins/<plugin>/configs/settings.jsonc`.
4. Restart the server, then run `volt list` in the console and confirm the plugin loaded.

Metamod loads one plugin, the VoltMod host; the host loads plugins from
`addons/voltmod/plugins/`. `meta list` shows the host, `volt list` shows the plugins.

Read the plugin's own guide before enabling it. The anticheat has a staged rollout and starts in
`observe` mode.

## Build it yourself

Windows with Git, [uv](https://docs.astral.sh/uv/), Python 3.14+, Visual Studio 2022 or newer with
the C++ workload, a local CS2 dedicated server and Metamod:Source.

```powershell
git clone https://github.com/voltygg/cs2-plugins.git
cd cs2-plugins
Copy-Item .env.example .env      # set CS2_SERVER_PATH
uv sync
uv run poe doctor
uv run poe bootstrap             # first build: Conan setup, configure, build, test
uv run poe build --install admin-system --start
```

| Command | What it does |
| --- | --- |
| `uv run poe doctor` | Check tools, project files, Conan setup and the CS2 server |
| `uv run poe bootstrap` | First-time setup, then configure, build and test |
| `uv run poe build [preset]` | Build; presets are `windows-msvc-{release,debug}` and `linux-steamrt-{release,debug}` |
| `uv run poe build-linux` | Build the Linux Steam Runtime preset (CI container only) |
| `uv run poe build --install <name> [--start]` | Build, install one plugin locally, optionally launch the server |
| `uv run poe install [name]` / `start-server` | Install and launch as separate steps |
| `uv run poe test` | Build, then run CTest |
| `uv run poe lint` / `format` | Ruff, module graph, Panorama and schema checks / C++ formatting |
| `uv run poe schema` | Regenerate the admin-system table specs from its migrations |
| `uv run poe new-plugin <name>` | Scaffold and register a plugin |
| `uv run poe panorama` | Compile the Panorama screens into your CS2 client (Windows) |
| `uv run poe deploy-*`, `rcon` | Remote deployment; see [Deployment](deploy/README.md) |

Build output lands in `build/<preset>/plugins/<name>/<platform-arch>/`. A local install keeps an
existing `settings.jsonc`, so it never overwrites your own changes.

## Docs

| Page | Covers |
| --- | --- |
| [Create your first plugin](docs/getting-started-plugin.md) | scaffold, build, install, verify |
| [Local development](docs/local-development.md) | toolchain, presets, the editable framework checkout, logs |
| [Admin system](docs/admin-system.md) | flags, database, multi-server setup |
| [Deployment](deploy/README.md) | inventory, secrets, panel servers and Docker hosts |
| [Contributing](CONTRIBUTING.md) | conventions and review expectations |
| [VoltMod docs](https://github.com/voltygg/voltmod/tree/main/docs) | the framework itself |

## Layout

```text
plugins/     plugin source, configs, tests and docs
docs/        contributor and operator guides
deploy/      the deploy CLI, inventory and host tooling
vendor/      an optional VoltMod checkout for coordinated framework changes
build/       generated output
```
