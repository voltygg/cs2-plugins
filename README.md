# Admin System

A modern C++ admin system plugin for Counter-Strike 2 community servers, built on Metamod:Source.

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
3. Configure database connection in `addons/admin-system/configs/database.json`
4. Add admins in `addons/admin-system/configs/admins.json`
5. Restart the server

## Commands

| Command | Permission | Description |
|---------|------------|-------------|
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

### database.json

```json
{
  "host": "localhost",
  "port": 5432,
  "database": "cs2_server",
  "username": "admin_system",
  "password": "your_password",
  "schema": "admin_system",
  "pool_size": 4
}
```

### admins.json

```json
{
  "admins": [
    {
      "steam_id": "76561198012345678",
      "name": "Server Owner",
      "groups": ["superadmin"],
      "immunity": 100
    }
  ]
}
```

### Quick Start (Docker)

```bash
# Clone with submodules
git clone --recursive <https://github.com/m9snoi/admin-system.git>

# Build Linux binary
docker compose run --rm build
```

### Windows Build

Requires Visual Studio 2026, vcpkg, and Python 3.14+ with PDM.

```bash
# Clone with submodules
git clone --recursive <https://github.com/m9snoi/admin-system.git>
cd admin-system

# Install dependencies
pdm install
vcpkg install

# Build and deploy
scripts/build.ps1
```

See [docs/local-development.md](docs/local-development.md) for full setup guide.
