# Admin System - CS2 Metamod Plugin

## Project Overview

C++23 Metamod:Source plugin for CS2 community servers providing admin functionality: player management, punishments (ban/mute/gag/warn), and WASD center-HTML menus.

Reusable engine abstractions (commands, menu, SDK wrappers, utilities) have been extracted into **[CS2-Kit](https://github.com/suxrobGM/cs2-kit)** (`vendor/cs2-kit/`), a standalone library. Admin-system consumes CS2-Kit as a vendored submodule compiled alongside plugin source.

## Tech Stack

- **Language:** C++23
- **Framework:** Metamod:Source 2.0 + hl2sdk-cs2
- **Shared Library:** CS2-Kit (vendor/cs2-kit) — commands, menus, SDK wrappers, utilities
- **Database:** PostgreSQL 18 via libpqxx
- **UI:** WASD center-HTML menus (W/S navigate, E select, R close)
- **Build System:** AMBuild (auto-discovers .cpp files from `src/` and `vendor/cs2-kit/src/`)

## Build Commands

```bash
# Local Windows (from x64 Native Tools Command Prompt)
scripts/build.ps1

# Deploy to CS2 server
scripts/deploy.ps1
```

## Project Structure

```text
src/
├── Core/
│   ├── Plugin.hpp/cpp          # ISmmPlugin entry point, hooks, subsystem init
│   ├── Config.hpp/cpp          # Loads settings.json + admins.json
│   └── Adapters.hpp/cpp        # CS2-Kit interface implementations (ILogger, IPathResolver, IMenuIO, ICommandCaller)
├── Players/
│   ├── Player.hpp/cpp          # Player data (slot, SteamID, name, flags)
│   └── PlayerManager.hpp/cpp   # Player lifecycle tracking
├── Admin/
│   ├── AdminManager.hpp/cpp    # Permissions, flags (bitmask), immunity
│   └── AdminMenu.hpp/cpp       # Admin-specific menu builders
├── Punishments/
│   └── PunishmentManager.hpp/cpp
├── Database/
│   ├── Database.hpp/cpp        # PostgreSQL connection pool
│   ├── Entities/               # Admin, AdminGroup, Ban, Mute, Gag, Warning, Player
│   └── Repositories/           # AdminRepository, BanRepository
vendor/
├── cs2-kit/                    # Reusable CS2 plugin library (submodule)
│   └── src/
│       ├── Commands/           # Command system (Command, CommandBuilder, CommandManager, ICommandCaller)
│       ├── Core/               # Singleton, Scheduler, ILogger, IPathResolver
│       ├── Menu/               # Menu system (Menu, MenuBuilder, MenuManager, MenuRenderer, IMenuIO)
│       ├── Sdk/                # SDK wrappers (GameInterfaces, GameData, Entity, Schema, SigScanner, ...)
│       └── Utils/              # SteamId, StringUtils, TimeUtils, Translations, Log
├── hl2sdk-cs2/                 # HL2SDK for Source 2
├── hl2sdk-manifests/           # SDK manifest definitions
└── mmsource-2.0/               # Metamod:Source 2.0

configs/
├── settings.json               # Plugin, database, commands, punishments config
├── admins.json                 # Groups + admins (merged)
└── translations/               # en.json, ru.json
gamedata/
└── signatures.jsonc            # Engine signatures and offsets
references/                     # Third-party project examples (read-only reference, do NOT modify or search extensively)
```

## Code Conventions

### Naming (C# style)

| Element | Convention | Example |
| ------- | ---------- | ------- |
| Namespaces | `PascalCase`, nested | `AdminSystem::Players` |
| Classes/Structs | `PascalCase` | `PlayerManager`, `MenuItem` |
| Methods | `PascalCase` | `GetPlayer()`, `HandleInput()` |
| Member variables | `_camelCase` | `_admins`, `_states` |
| Constants | `PascalCase` | `MaxPlayers`, `InputDebounceMs` |
| Parameters/locals | `camelCase` | `steamId`, `targetSlot` |
| JSON keys | `camelCase` | `steamId`, `adminPanel` |

### C++ Style

- **C++17 nested namespaces:** `namespace AdminSystem::Sdk { ... }`
- **`.hpp` headers** (not `.h`)
- **CRTP Singleton** (`CS2Kit::Core::Singleton<T>`) for all singletons
- **No mutexes** (main-thread-only design; all Metamod hooks run on game thread)
- **Designated initializers** for struct construction
- **`std::format`** for string formatting
- **`int64_t`** for SteamIDs
- **`uint32_t` bitmask** for admin flags (a=bit0, b=bit1, ..., z=bit25)
- **Builder pattern** for Menu and Command construction (via CS2-Kit)

## Config Files

- **`settings.json`**: All plugin configuration (plugin, database, commands, punishments, admin sections)
- **`admins.json`**: Admin groups and admin entries (merged file)
- **`signatures.jsonc`**: Engine signatures/offsets with platform-specific pattern+offset pairs

## Admin Flags

Single char a-z, stored as `uint32_t` bitmask for O(1) checks:

- `a` = reserved slot, `b` = generic admin, `c` = kick, `d` = ban
- `o` = mute, `p` = gag, `q` = warn, `r` = admin menu access
- `z` = root (all permissions)

## Database

- PostgreSQL with schema: `admin_system`
- Tables: admins, admin_groups, players, bans, mutes, gags, warnings, audit_log
- Prepared statements for all queries
- Mutex only in Database class (future async support)
