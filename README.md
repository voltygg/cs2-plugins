# Admin System

A modern C++ admin system plugin for Counter-Strike 2 community servers, built on Metamod:Source.

Built on top of **[CS2Kit](https://github.com/suxrobGM/cs2-kit)** — a reusable C++23 library for CS2 plugin development providing commands, menus, SDK wrappers, and utilities. CS2Kit is the only submodule — all SDK dependencies (hl2sdk-cs2, mmsource-2.0, etc.) are nested inside it.

## Features

- **Punish:** Kick, ban, mute, gag, warn
- **Control:** Slay, Bring, Goto, Freeze, Noclip, Health/Armor presets, Godmode (FL_GODMODE), Bury/Unbury, Change Team
- **Effects:** Ghost (translucent render), Disco (color cycling), Launch (high-velocity yeet + 3 s fall protect), Smite (theatrical instakill), Swap (exchange two players' positions). Blind reserved (awaits Fade user-message infra).
- **Admin System:** Permission flags (`s` control, `h` survival/cheats, `f` fun), groups, immunity levels. Self-targeting always allowed.
- **WASD Menus:** Top-level category dispatcher → player picker → actions. Toggle entries (Ghost, Disco, Godmode) show live `: ON / : OFF` state via dynamic-title menu items.
- **Database:** PostgreSQL with async queries
- **Chat Commands:** `!kick`, `!ban`, `!voice_mute`, `!text_mute`, `!warn` and more

## Requirements

- CS2 Dedicated Server
- [Metamod:Source 2.0](https://www.sourcemm.net/)
- PostgreSQL 18+

## Installation

1. Download the latest release from [Releases](#)
2. Extract to your server's `csgo/` folder
3. Configure database and plugin settings in `addons/admin-system/configs/settings.jsonc`
4. (Optional) The plugin applies the schema automatically on load. To pre-create it manually: `psql -d admin_system -f configs/migrations/0001_initial_schema.sql`
5. Edit `database/seed-admin.sql` with your SteamID64 and run it: `psql -d admin_system -f database/seed-admin.sql`
6. Restart the server (or run `!admin_reload` if it was already running)

## Commands

| Command | Permission | Description |
| --- | --- | --- |
| `!kick <target> [reason]` | Kick | Kick a player |
| `!ban <target> <duration> [reason]` | Ban | Ban a player |
| `!unban <steamid>` | Unban | Remove a ban |
| `!voice_mute <target> <duration> [reason]` | VoiceMute | Mute voice |
| `!voice_unmute <target>` | VoiceMute | Unmute voice |
| `!text_mute <target> <duration> [reason]` | TextMute | Block text chat |
| `!text_unmute <target>` | TextMute | Unblock text chat |
| `!warn <target> <reason>` | Warn | Issue a warning |
| `!admin` | AdminMenu | Open admin menu |

**Target Selectors:** `@all`, `@me`, `@ct`, `@t`, or partial player name

**Duration Format:** `5m`, `1h`, `1d`, `1w`, or `0` for permanent

## Configuration

Runtime configuration lives in `configs/settings.jsonc` (database, punishments, chat, cheat-check). Admin groups (with their chat prefix and colors) and individual admins live in the `admin_groups` and `admins` PostgreSQL tables -- see `configs/migrations/`. Run `!admin_reload` after editing those tables to refresh in-memory state without restarting.

### Quick Start (Docker)

```bash
# Clone with submodules (--recursive pulls CS2Kit and its nested SDK submodules)
git clone --recursive https://github.com/m9snoi/admin-system.git

# Build Linux binary
docker compose run --rm build
```

### Windows Build

Requires Visual Studio 2026, vcpkg, and Python 3.14+ with uv.

```bash
# Clone with submodules
git clone --recursive https://github.com/m9snoi/admin-system.git
cd admin-system

# Install dependencies
uv sync
vcpkg install

# Build and deploy
scripts/build.sh
```

See [docs/local-development.md](docs/local-development.md) for full setup guide.
