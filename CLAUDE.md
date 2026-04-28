# Admin System - CS2 Metamod Plugin

## Project Overview

C++23 Metamod:Source plugin for CS2 community servers providing admin functionality: player management, punishments (ban/mute/gag/warn), and WASD center-HTML menus.

Reusable engine abstractions (commands, menu, SDK wrappers, utilities) live in **[CS2Kit](https://github.com/suxrobGM/cs2-kit)** (`vendor/cs2-kit/`), a standalone library with its own build system and SDK submodules. Admin-system consumes CS2Kit via source inclusion (compiles `.cpp` files inline into the plugin binary).

## Tech Stack

- **Language:** C++23
- **Framework:** Metamod:Source 2.0 + hl2sdk-cs2
- **Shared Library:** CS2Kit (vendor/cs2-kit) — commands, menus, SDK wrappers, utilities
- **Database:** PostgreSQL 18 via libpqxx
- **UI:** WASD center-HTML menus (W/S navigate, E select, R close)
- **Build System:** AMBuild (auto-discovers .cpp files from `src/` and `vendor/cs2-kit/src/`)

## Build Commands

```bash
# Local Windows (run via Git Bash from an x64 Native Tools shell)
scripts/build.sh

# Deploy to CS2 server
scripts/deploy.sh
```

## Project Structure

```text
src/
├── Core/
│   ├── Plugin.hpp/cpp          # ISmmPlugin entry point, hooks, CS2Kit::Initialize()
│   └── Config.hpp/cpp          # Loads settings.json (admin groups & entries live in the DB)
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
    │   ├── Players/            # Player (identity + connection), PlayerManager (slot/steamid lookup)
    │   ├── Sdk/                # GameInterfaces, GameData, Entity, Schema, SigScanner, ...
    │   └── Utils/              # SteamId, StringUtils, TimeUtils, Translations, Log
    ├── src/                    # Implementation (.cpp files)
    └── vendor/                 # SDK submodules (shared, no duplicates in admin-system)
        ├── hl2sdk-cs2/         # HL2SDK for Source 2
        ├── hl2sdk-manifests/   # SDK manifest definitions
        ├── mmsource-2.0/       # Metamod:Source 2.0
        └── nlohmann/           # nlohmann/json (single-include)

configs/
├── settings.json               # Plugin, database, punishments, chat config
└── translations/               # en.json, ru.json
database/
└── schema.sql                  # PostgreSQL schema; admin_groups + admins seeded here
references/                     # Third-party project examples (read-only reference, do NOT modify or search extensively)
```

## CS2Kit Integration

Admin-system uses **source inclusion**: cs2-kit `.cpp` files are compiled directly into the plugin binary. The AMBuild auto-discovers sources from `vendor/cs2-kit/src/`. All SDK dependencies (hl2sdk-cs2, hl2sdk-manifests, mmsource-2.0, nlohmann/json) live inside cs2-kit's `vendor/` — admin-system has no duplicate submodules.

Include style: `#include <CS2Kit/Commands/Command.hpp>`

### Initialization Flow

Plugin.cpp calls `CS2Kit::Initialize(ismm, error, maxlen, params)` which resolves all SDK interfaces internally via Metamod's `ISmmAPI`, loads built-in gamedata, sets `g_pCVar`, and initializes all subsystems. The plugin drives the player lifecycle by calling `PlayerManager::AddPlayer`/`RemovePlayer` from its connect/disconnect hooks; command handlers receive `CS2Kit::Players::Player*` directly.

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
- **Builder pattern** for Menu and Command construction (via CS2Kit)

### Comments

Default to writing **no comments**. Names should carry the meaning. Add a comment only when the *why* is non-obvious — a hidden constraint, a workaround for a specific engine quirk, behavior that would surprise a reader. Keep them to **one or two short lines**. Don't restate what the code does, don't reference the current task or PR, and don't write multi-paragraph docstrings.

### File Size

Aim to keep source files under **~300-350 LOC**. When a file grows past that, split by responsibility (e.g. extract a sibling helper, move a sub-feature into its own translation unit). Headers stay small and focused; large monolithic `.cpp` files should be the exception, not the norm.

## Config Files

- **`configs/settings.json`**: Plugin/database/punishments/chat configuration. All non-admin runtime knobs live here.
- **`database/schema.sql`**: Owns the `admin_groups` and `admins` tables. Groups (with their chat prefix/colors) and individual admins are managed in PostgreSQL -- no JSON admin file. Use `!admin_reload` to pick up DB changes without a restart.

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
