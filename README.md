# CS2 plugins

Native C++23 plugins for Counter-Strike 2, built on the
[VoltMod framework](vendor/voltmod/README.md).

## Plugins

| Plugin | What it provides | Runtime dependency |
| --- | --- | --- |
| [Admin system](plugins/admin-system/README.md) | Permissions, moderation, reports, player effects, and cheat checks | PostgreSQL |
| [Anticheat](plugins/anticheat/README.md) | Server-side movement and aim detection with observe, kick, and ban modes | PostgreSQL |
| [Bhop](plugins/bhop/README.md) | Client-predicted autobhop, movement tuning, and per-player grants | Admin system for grants mode |

## Install a release

You need a CS2 dedicated server and
[Metamod:Source 2](https://www.sourcemm.net/downloads.php/?branch=master).
The admin system and anticheat also need PostgreSQL.

1. Download the required plugin archive from the repository releases.
2. Extract it into the server's `game/csgo` directory.
3. Edit the plugin configuration under `game/csgo/addons/voltmod/configs`.
4. Restart the server.

Read the plugin guide before enabling it. Each guide documents its dependencies,
configuration, commands, and rollout notes.

## Build from source

Clone the repository with submodules:

```bash
git clone --recurse-submodules <repository-url>
cd cs2-plugins
```

### Windows

Install Python 3.12+, [uv](https://docs.astral.sh/uv/), CMake, Ninja, Conan,
and a C++23 compiler. Then run:

```powershell
uv sync
uv run poe bootstrap
uv run poe build
```

`bootstrap` installs the Conan profiles and dependencies required by the
configured build. Run it after cloning and whenever dependency definitions or
profiles change.

### Linux

The repository includes a Steam Runtime container build:

```bash
docker build -f build/linux/Dockerfile -t cs2-plugins-builder .
docker run --rm -v "${PWD}:/src" cs2-plugins-builder
```

Build artifacts are written under `build/`.

## Common development commands

| Command | Purpose |
| --- | --- |
| `uv run poe bootstrap` | Prepare Conan profiles and dependencies |
| `uv run poe build` | Build for the current platform |
| `uv run poe build-linux` | Build the Linux Steam Runtime target |
| `uv run poe lint` | Run repository checks |
| `uv run poe format` | Format supported source files |
| `uv run poe new-plugin <name>` | Scaffold a plugin |

## Repository layout

- `plugins/` contains the plugin source, configuration, and documentation.
- `deploy/` contains production deployment tooling and inventory.
- `build/` contains container and toolchain files.
- `vendor/voltmod/` contains the VoltMod framework submodule.

To add a plugin, run `uv run poe new-plugin <name>` and add its directory to the
root `CMakeLists.txt`.

See [CONTRIBUTING.md](CONTRIBUTING.md) for development conventions and
[deploy/README.md](deploy/README.md) for server deployment.
