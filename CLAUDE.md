# Admin System - CS2 Metamod Plugin

## Project Overview

C++ Metamod:Source plugin for CS2 community servers providing comprehensive admin functionality including player management, punishments, and a stunning in-game UI.

## Tech Stack

- **Language:** C++23
- **Framework:** Metamod:Source 2.0 + hl2sdk-cs2
- **Database:** PostgreSQL 18 via libpqxx
- **UI:** WASD center-HTML menus
- **Build System:** AMBuild

## Build Commands

```bash
# Local Windows (from x64 Native Tools Command Prompt)
cd scripts
./build.sh
```

## Project Structure

```text
src/
├── core/           # Plugin infrastructure (ISmmPlugin, events, hooks)
├── sdk/            # SDK wrappers (entity access, user messages)
├── player/         # Player tracking and management
├── admin/          # Admin levels, permissions, groups
├── commands/       # Chat command system (.kick, !ban, etc.)
├── punishments/    # Kick, ban, mute, gag, warnings
├── database/       # PostgreSQL integration, async queries
├── menus/          # WASD menu system (center HTML)
└── utils/          # Common utilities (SteamID, time, strings)

configs/            # Plugin configuration (JSON format)
gamedata/           # Signatures and offsets
```

## Code Conventions

- **C++ Version**: Use modern C++23 features where supported
  - Prefer C++23 > C++20 features (ranges, std::expected, std::print)
  - Use ranges and views for collections
  - Use designated initializers
  - Use std::format for string formatting
- **Code Style**:
  - Follow existing patterns in codebase
  - Entity classes in `src/database/entities/`
  - Repository pattern for database access
  - JSON for all configuration files
  - Prefer `std::optional` over null pointers
  - Use `int64_t` for SteamIDs

## Database

- PostgreSQL with dedicated schema: `admin_system`
- Tables: admins, admin_groups, players, bans, mutes, gags, warnings, audit_log
- Use prepared statements for all queries (SQL injection prevention)
- Async queries for non-blocking operations

## Testing Workflow

1. Start PostgreSQL: `docker compose up -d postgres`
2. Build plugin: `docker compose run --rm build`
3. Copy `build/package/` contents to CS2 server's `csgo/` folder
4. Start server: `scripts/start-server.ps1`
5. Connect with CS2 client and test commands

## Key Classes

- `AdminSystemPlugin` - Main plugin entry point (ISmmPlugin)
- `PlayerManager` - Tracks connected players
- `AdminManager` - Manages admin cache and permissions
- `CommandManager` - Registers and dispatches chat commands
- `PunishmentManager` - Handles ban/mute/gag/warn enforcement
- `Database` - PostgreSQL connection pool and async queries
- `MenuManager` - WASD-style center HTML menu system
