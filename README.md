# Admin System

A modern C++ admin system plugin for Counter-Strike 2 community servers, built on Metamod:Source.

Built on top of **[CS2-Kit](https://github.com/suxrobGM/cs2-kit)** — a reusable C++23 library for CS2 plugin development providing commands, menus, SDK wrappers, and utilities. CS2-Kit is the only submodule — all SDK dependencies (hl2sdk-cs2, mmsource-2.0, etc.) are nested inside it.

## Features

- **Player Management:** Kick, ban, mute, gag, warn
- **Admin System:** Permission flags, groups, immunity levels
- **WASD Menus:** In-game center-HTML menus with W/S/E/R navigation
- **Database:** PostgreSQL with async queries
- **Chat Commands:** `!kick`, `!ban`, `!mute`, `!gag`, `!warn` and more

## Requirements

- CS2 Dedicated Server
- [Metamod:Source 2.0](https://www.sourcemm.net/)
- PostgreSQL 18+

## Installation

1. Download the latest release from [Releases](#)
2. Extract to your server's `csgo/` folder
3. Configure database and plugin settings in `addons/admin-system/configs/settings.json`
4. Add admins in `addons/admin-system/configs/admins.json`
5. Restart the server

## Commands

| Command | Permission | Description |
| --- | --- | --- |
| `!kick <target> [reason]` | Kick | Kick a player |
| `!ban <target> <duration> [reason]` | Ban | Ban a player |
| `!unban <steamid>` | Unban | Remove a ban |
| `!mute <target> <duration> [reason]` | Mute | Mute voice |
| `!unmute <target>` | Mute | Unmute voice |
| `!gag <target> <duration> [reason]` | Gag | Gag text chat |
| `!ungag <target>` | Gag | Ungag text chat |
| `!warn <target> <reason>` | Warn | Issue a warning |
| `!admin` | AdminMenu | Open admin menu |

**Target Selectors:** `@all`, `@me`, `@ct`, `@t`, or partial player name

**Duration Format:** `5m`, `1h`, `1d`, `1w`, or `0` for permanent

## Configuration

All plugin configuration is in `configs/settings.json` (database, commands, punishments, etc.). Admin groups and entries are defined in `configs/admins.json`.

### Quick Start (Docker)

```bash
# Clone with submodules (--recursive pulls CS2-Kit and its nested SDK submodules)
git clone --recursive https://github.com/m9snoi/admin-system.git

# Build Linux binary
docker compose run --rm build
```

### Windows Build

Requires Visual Studio 2026, vcpkg, and Python 3.14+ with PDM.

```bash
# Clone with submodules
git clone --recursive https://github.com/m9snoi/admin-system.git
cd admin-system

# Install dependencies
pdm install
vcpkg install

# Build and deploy
scripts/build.sh
```

See [docs/local-development.md](docs/local-development.md) for full setup guide.
