# Admin System - CS2 Metamod Plugin

## Project Overview

C++23 Metamod:Source plugin for CS2 community servers providing admin functionality: player management, punishments (ban/mute/gag/warn), and WASD center-HTML menus.

Reusable engine abstractions (commands, menu, SDK wrappers, utilities) live in **[CS2-Kit](https://github.com/suxrobGM/cs2-kit)** (`vendor/cs2-kit/`), a standalone library with its own build system and SDK submodules. Admin-system consumes CS2-Kit via source inclusion (compiles `.cpp` files inline into the plugin binary).

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
│   ├── Plugin.hpp/cpp          # ISmmPlugin entry point, hooks, CS2Kit::Initialize()
│   ├── Config.hpp/cpp          # Loads settings.json + admins.json
│   └── PlayerCaller.hpp/cpp    # ICommandCaller adapter (wraps Player* for command system)
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
└── cs2-kit/                    # Reusable CS2 plugin library (only submodule)
    ├── include/CS2Kit/         # Public headers (#include <CS2Kit/...>)
    │   ├── CS2Kit.hpp          # Initialization API (InitParams, Initialize/Shutdown)
    │   ├── Commands/           # Command, CommandBuilder, CommandManager, ICommandCaller
    │   ├── Core/               # Singleton, Scheduler, ILogger, ConsoleLogger, Paths
    │   ├── Menu/               # Menu, MenuBuilder, MenuManager, MenuRenderer
    │   ├── Sdk/                # GameInterfaces, GameData, Entity, Schema, SigScanner, ...
    │   └── Utils/              # SteamId, StringUtils, TimeUtils, Translations, Log
    ├── src/                    # Implementation (.cpp files)
    └── vendor/                 # SDK submodules (shared, no duplicates in admin-system)
        ├── hl2sdk-cs2/         # HL2SDK for Source 2
        ├── hl2sdk-manifests/   # SDK manifest definitions
        ├── mmsource-2.0/       # Metamod:Source 2.0
        └── nlohmann/           # nlohmann/json (single-include)

configs/
├── settings.json               # Plugin, database, commands, punishments config
├── admins.json                 # Groups + admins (merged)
└── translations/               # en.json, ru.json
gamedata/
└── signatures.jsonc            # Engine signatures and offsets
references/                     # Third-party project examples (read-only reference, do NOT modify or search extensively)
```

## CS2-Kit Integration

Admin-system uses **source inclusion**: cs2-kit `.cpp` files are compiled directly into the plugin binary. The AMBuild auto-discovers sources from `vendor/cs2-kit/src/`. All SDK dependencies (hl2sdk-cs2, hl2sdk-manifests, mmsource-2.0, nlohmann/json) live inside cs2-kit's `vendor/` — admin-system has no duplicate submodules.

Include style: `#include <CS2Kit/Commands/Command.hpp>`

### Initialization Flow

Plugin.cpp populates `CS2Kit::InitParams` with SDK interfaces (via `GET_V_IFACE` macros) and calls `CS2Kit::Initialize(params)`. This replaces the old adapter pattern — cs2-kit ships a built-in `ConsoleLogger` and internal path resolution, so the only adapter needed is `PlayerCaller` (implements `ICommandCaller` to bridge `Player*` into the command system).

Hook callbacks:
- `Hook_GameFrame` → `CS2Kit::OnGameFrame()` (drives Scheduler + MenuManager)
- `Hook_ClientDisconnect` → `CS2Kit::OnPlayerDisconnect(slot)` (cleans menu state)
- `Hook_DispatchConCommand` → dispatches chat messages to `CommandManager`

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
