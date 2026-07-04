# CS2 Plugins

A C++23 Metamod:Source plugin monorepo for Counter-Strike 2 community servers. Each plugin lives under `plugins/<name>/` with its own sources, configs, and CMake target.

Built on top of **[CS2Kit](https://github.com/suxrobGM/cs2-kit)** - a reusable C++23 library for CS2 plugin development (declarative commands, WASD menus, async PostgreSQL, engine SDK wrappers) - consumed as the `vendor/cs2-kit` submodule. CS2Kit is the only submodule; all SDK dependencies (hl2sdk-cs2, mmsource-2.0, etc.) are nested inside it.

## Plugins

| Plugin | Description |
| --- | --- |
| [admin-system](plugins/admin-system/README.md) | Full admin suite: punishments, fun effects, WASD admin menu, multi-server admin groups, abuse protection, cheat-check |

## Requirements

- CS2 Dedicated Server
- [Metamod:Source 2.0](https://www.sourcemm.net/)
- PostgreSQL 18+ (for plugins that use the database)

## Installation

Grab the latest build from [Releases](https://github.com/m9snoi-net/cs2-plugins/releases) and extract it into your server's `csgo/` folder. Per-plugin configuration is covered in each plugin's README.

## Building

### Linux (Docker)

```bash
# Clone with submodules (--recursive pulls CS2Kit and its nested SDK submodules)
git clone --recursive https://github.com/m9snoi-net/cs2-plugins.git
cd cs2-plugins

docker compose -f deploy/docker-compose.build.yml run --rm --build build
```

### Windows

Requires Visual Studio 2026 Build Tools. CMake 4.3.4+, Conan 2.29.1+, and Ninja
are pinned in `pyproject.toml`, so `uv sync` installs them (or install globally).

```bash
git clone --recursive https://github.com/m9snoi-net/cs2-plugins.git
cd cs2-plugins

uv run poe build
```

The default Windows preset is `windows-msvc-release`; use
`uv run poe build windows-msvc-debug` for a debug build. Output lands in
`build/<preset>/plugins/<name>/<platform-arch>/`.

See [docs/local-development.md](docs/local-development.md) for the full setup
guide and [CONTRIBUTING.md](CONTRIBUTING.md) for the development workflow.

## Adding a plugin

```bash
uv run poe new-plugin my-plugin
```

Stamps `plugins/my-plugin/` from CS2Kit's template tree (PluginBase skeleton,
settings.jsonc, translations, an example `!ping` command) and registers it in
the root `CMakeLists.txt`. It builds and loads as-is.
