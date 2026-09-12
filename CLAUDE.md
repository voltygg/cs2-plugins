# CS2 plugins

C++23 Metamod:Source plugins for Counter-Strike 2 on the VoltMod framework
(Conan package `voltmod/[~1.4]`).

- `vendor/voltmod` is a separate Git repo with its own `CLAUDE.md`. Check its status and diffs separately.
- `references/` is read-only.

## Comments and names

MUST:

- Comment only where the code cannot say it: intent, ownership, lifetime, threading, compatibility.
- One line inside a function. A public contract may take a few lines of Doxygen. No essays.
- Names are plain words a new developer understands. If a name needs a comment to decode it, rename it.

## Commands

```bash
uv run poe doctor                            # check toolchain, Conan, optional CS2 server
uv run poe bootstrap                         # first-time: install VoltMod profiles and remote
uv run poe build                             # compile (windows-msvc-release)
uv run poe test                              # compile, then CTest
uv run poe lint                              # ruff + modgraph + panorama and schema checks
uv run poe schema                            # render the admin-system migrations and table specs
uv run poe build --install <plugin> --start  # copy to CS2_SERVER_PATH and launch
uv run poe panorama                          # render, compile and install the Panorama screens into your client (Windows)
uv run poe new-plugin <name>
```

Output: `build/<preset>/plugins/<name>/<platform-arch>/`. Compiling needs an MSVC dev
shell; see the `/build-local` skill. `build-linux` only works in the CI container.

Framework and plugin together:

```bash
uv run conan editable add vendor/voltmod   # once
uv run poe build                           # checkout first, then plugins
uv run poe build --relock                  # before committing; commit conan.lock with the change
```

## Layout

```text
plugins/admin-system/  Admins, punishments, menus, reports, Postgres/MariaDB/SQLite
plugins/anticheat/     Detection cores, engine adapters, responses
plugins/bhop/          Bunnyhop modes
plugins/contracts/     Interfaces shared between plugins
plugins/ui/            Panorama HUD, published as Contracts::IUiHud
deploy/                Docker deployment CLI
docs/                  Development and deployment notes
```

A plugin owns `src/`, `configs/`, `tests/`, and a `CMakeLists.txt` calling
`voltmod_add_plugin(<name> VERSION <v>)`; register it with `add_subdirectory()` in the
root. C++ deps: `conanfile.py`, then `find_package` in the root CMake, then link in the plugin.

Conventions and framework patterns are in `.claude/rules/` and load per file path.
