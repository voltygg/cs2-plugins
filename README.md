# CS2 plugins

C++23 Metamod:Source plugins for Counter-Strike 2 community servers. Each
plugin has its own source, configuration, and CMake target under `plugins/`.

The plugins use [VoltMod](https://github.com/voltygg/voltmod) for commands, WASD
menus, asynchronous PostgreSQL, and Source SDK integration. Conan provides
VoltMod, HL2SDK, and Metamod. The optional `vendor/voltmod` submodule supports
developing the framework and plugins together.

## Plugins

| Plugin | Description |
| --- | --- |
| [admin-system](plugins/admin-system/README.md) | Admin commands and menu, punishments, fun effects, shared admin groups, abuse protection, and cheat-check workflows |
| [bhop](plugins/bhop/README.md) | Client-predicted auto-bunnyhop with air acceleration and optional per-player session grants from admin-system |

## Requirements

- CS2 Dedicated Server
- [Metamod:Source 2.0](https://www.sourcemm.net/)
- PostgreSQL 18+ (for plugins that use the database)

## Install a release

Download the latest build from [Releases](https://github.com/voltygg/cs2-plugins/releases)
and extract it into the server's `csgo/` folder. Each plugin README describes
its configuration.

## Building

### Linux with Docker

```bash
git clone https://github.com/voltygg/cs2-plugins.git
cd cs2-plugins

docker compose -f deploy/docker-compose.build.yml run --rm --build build
```

### Windows

Visual Studio 2026 Build Tools is required. The `voltmod` distribution pins
CMake 4.3.4+, Conan 2.29.1+, and Ninja, so `uv sync` installs them.

```bash
git clone https://github.com/voltygg/cs2-plugins.git
cd cs2-plugins

uv sync
uv run poe bootstrap   # install Conan profiles and the remote, then build once
uv run poe build       # subsequent builds
```

The default Windows preset is `windows-msvc-release`. Use
`uv run poe build windows-msvc-debug` for a debug build. Output is written to
`build/<preset>/plugins/<name>/<platform-arch>/`.

See [docs/local-development.md](docs/local-development.md) for the full setup
guide and [CONTRIBUTING.md](CONTRIBUTING.md) for the development workflow.

## Add a plugin

```bash
uv run poe new-plugin my-plugin
```

This creates and registers `plugins/my-plugin/` with a plugin skeleton,
`settings.jsonc`, translations, and an example `!ping` command.
