# CS2 plugins

C++23 Metamod:Source plugins for Counter-Strike 2 community servers. Each plugin
has its own source, configuration, and CMake target under `plugins/`.

VoltMod provides the shared command, menu, PostgreSQL, and Source SDK layers.
Conan supplies VoltMod, HL2SDK, and Metamod. The optional `vendor/voltmod`
checkout supports changing the framework and plugins together.

## Plugins

| Plugin | Purpose |
| --- | --- |
| [admin-system](plugins/admin-system/README.md) | Admin commands and menus, punishments, effects, groups, abuse protection, and player reports |
| [anticheat](plugins/anticheat/README.md) | Server-side aim, input, client-convar, and integrity detections |
| [bhop](plugins/bhop/README.md) | Client-predicted bunnyhop with server-wide and per-player modes |

## Requirements

- CS2 Dedicated Server
- [Metamod:Source 2.0](https://www.sourcemm.net/)
- PostgreSQL 18+ for plugins that use a database

## Install a release

Download a build from [Releases](https://github.com/voltygg/cs2-plugins/releases)
and extract it into the server's `csgo/` directory. Configure each plugin as
described in its README.

## Build

On Linux, build in the Steam Runtime container:

```bash
git clone https://github.com/voltygg/cs2-plugins.git
cd cs2-plugins
docker compose -f deploy/docker-compose.build.yml run --rm --build build
```

On Windows, install Visual Studio Build Tools with the C++ workload, then run:

```bash
git clone https://github.com/voltygg/cs2-plugins.git
cd cs2-plugins
uv sync
uv run poe bootstrap
```

After the first build, use `uv run poe build`. The default Windows preset is
`windows-msvc-release`; pass `windows-msvc-debug` for a debug build. Output is
written to `build/<preset>/plugins/<name>/<platform-arch>/`.

See [local development](docs/local-development.md) for setup details and
[CONTRIBUTING.md](CONTRIBUTING.md) for the development workflow.

## Add a plugin

```bash
uv run poe new-plugin my-plugin
```

The command creates `plugins/my-plugin/`, registers its CMake subdirectory, and
adds a working `!ping` example.
