# CS2 plugins

C++23 Metamod:Source plugins for Counter-Strike 2 community servers. Each
plugin lives under `plugins/<name>/` with its own source, configuration, and
CMake target.

The plugins use [VoltMod](https://github.com/voltygg/voltmod), a reusable C++23
framework that provides declarative commands, WASD menus, asynchronous
PostgreSQL, and Source SDK wrappers. VoltMod, the HL2SDK, and Metamod are Conan
packages from a public remote. The `vendor/voltmod` submodule is for developing
the framework and these plugins together; it is not required for a normal build.

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

This stamps `plugins/my-plugin/` from VoltMod's template tree with a
`MetamodPlugin` skeleton, `settings.jsonc`, translations, and an example
`!ping` command. It also registers the plugin in the root `CMakeLists.txt`, so
the generated plugin builds and loads without further wiring.
