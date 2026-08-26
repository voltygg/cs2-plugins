# CS2 plugins

Native C++23 plugins for Counter-Strike 2, built on the
[VoltMod framework](https://github.com/voltygg/voltmod).

## Choose your path

| I want to... | Start here |
| --- | --- |
| Install an existing plugin | [Install a release](#install-a-release) |
| Build or change a plugin in this repository | [Create your first plugin](docs/getting-started-plugin.md) |
| Build a standalone VoltMod project | [VoltMod getting started](https://github.com/voltygg/voltmod/blob/main/docs/getting-started.md) |
| Deploy production servers | [Deployment guide](deploy/README.md) |
| Contribute a change | [Contributing guide](CONTRIBUTING.md) |

## Plugins

| Plugin | What it provides | Runtime dependency |
| --- | --- | --- |
| [Admin system](plugins/admin-system/README.md) | Permissions, moderation, reports, effects, and cheat checks | PostgreSQL |
| [Anticheat](plugins/anticheat/README.md) | Movement, aim, and client-integrity detection | Admin system for alerts and bans; detection still runs without it |
| [Bhop](plugins/bhop/README.md) | Client-predicted autobhop and movement tuning | Admin system only in grants mode |

## Install a release

You need a CS2 dedicated server and
[Metamod:Source 2](https://www.sourcemm.net/downloads.php/?branch=master).
The admin system also needs PostgreSQL.

1. Download the required plugin archive from the repository releases.
2. Extract it into the server's `game/csgo` directory.
3. Edit its settings under `game/csgo/addons/<plugin>/configs`.
4. Restart the server.
5. Run `meta list` in the server console and confirm the plugin appears.

Read the plugin's guide before enabling it. The anticheat has a staged rollout
and should begin in `observe` mode.

## Develop a plugin locally

The supported Windows path needs Git, [uv](https://docs.astral.sh/uv/),
Python 3.14+, Visual Studio 2022 or newer with C++ tools, a local CS2 dedicated
server, and Metamod:Source.

The build resolves VoltMod and its SDK dependencies from Conan. A VoltMod
source checkout or Git submodule is not required.

```powershell
git clone https://github.com/voltygg/cs2-plugins.git
cd cs2-plugins
Copy-Item .env.example .env
uv sync
uv run poe doctor --server-path C:/cs2-server
uv run poe bootstrap
```

`bootstrap` installs the Conan profiles and package remote, resolves
dependencies, configures CMake, builds, and runs tests. It is the first build;
do not run `poe build` immediately afterward.

Create, build, install, and launch a working plugin:

```powershell
uv run poe new-plugin hello-world
uv run poe build --install hello-world --start
```

The scaffold registers itself in `CMakeLists.txt` and includes a `!ping`
command. After the server starts:

1. Run `meta list` and confirm `hello-world` is loaded.
2. Join the server and enter `!ping`.
3. Confirm the plugin replies with its translated pong message.

See [Create your first plugin](docs/getting-started-plugin.md) for the generated
files, configuration, tests, troubleshooting, and the normal edit-build-reload
loop.

## Common commands

| Command | Purpose |
| --- | --- |
| `uv run poe doctor` | Check tools, project files, Conan setup, and an optional CS2 server |
| `uv run poe bootstrap` | Prepare Conan, then configure, build, and test |
| `uv run poe build` | Run the normal release build |
| `uv run poe test` | Build, then run the test suite |
| `uv run poe build windows-msvc-debug` | Build the Windows debug preset |
| `uv run poe build-linux` | Build in the Linux Steam Runtime target |
| `uv run poe new-plugin <name>` | Scaffold and register a buildable plugin |
| `uv run poe build --install <name>` | Build, then install one plugin locally |
| `uv run poe build --install <name> --start` | Build, install, and launch the local server |
| `uv run poe lint` | Check Python source |
| `uv run poe format` | Apply the pinned C++ formatting |

Build output is written to
`build/<preset>/plugins/<name>/<platform-arch>/`. Local installation preserves
an existing plugin `settings.jsonc` so development deploys do not overwrite
operator changes.

## Repository layout

- `plugins/` contains plugin source, configuration, tests, and documentation.
- `docs/` contains contributor and local-development guides.
- `deploy/` contains local and production deployment tooling.
- `build/` contains generated output and toolchain files.
- `vendor/voltmod/` is an optional framework checkout for coordinated changes.

VoltMod, HL2SDK, and Metamod are resolved through Conan. The
`vendor/voltmod` checkout is not required to build this repository unless you
are changing the framework and a plugin together.

For a feature-level comparison with SwiftlyS2, Plugify, and
CounterStrikeSharp, see
[Choosing a CS2 plugin framework](https://github.com/voltygg/voltmod/blob/main/docs/framework-comparison.md).
