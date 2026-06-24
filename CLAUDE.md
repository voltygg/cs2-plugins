# Admin System - CS2 Metamod Plugin

## Project Overview

C++23 Metamod:Source plugin for CS2 community servers providing admin functionality: player management, punishments (ban/mute/gag/warn), and WASD center-HTML menus.

Reusable engine abstractions (commands, menu, SDK wrappers, utilities) live in **[CS2Kit](https://github.com/suxrobGM/cs2-kit)** (`vendor/cs2-kit/`), a standalone library with its own build system and SDK submodules. Each plugin consumes CS2Kit via source inclusion (compiles `.cpp` files inline into the plugin binary).

This repository is a **monorepo of plugins**: each lives under `plugins/<name>/` (its own `src/`, `configs/`, `<name>.vdf`), all sharing the single `vendor/cs2-kit` submodule and one root build that discovers each `plugins/<name>/src/AMBuilder`. `admin-system` is the first; add another by creating `plugins/<new>/` and appending deps to the root `vcpkg.json`.

## Tech Stack

- **Language:** C++23
- **Framework:** Metamod:Source 2.0 + hl2sdk-cs2
- **Shared Library:** CS2Kit (vendor/cs2-kit) — commands, menus, SDK wrappers, utilities
- **Database:** PostgreSQL 18 via libpqxx
- **UI:** WASD center-HTML menus (W/S navigate, E select, R close)
- **Build System:** AMBuild (auto-discovers .cpp files from each `plugins/<name>/src/` and `vendor/cs2-kit/src/`)

## Build Commands

```bash
# Local Windows (run via Git Bash from an x64 Native Tools shell)
scripts/build.sh

# Deploy to CS2 server
scripts/deploy.sh
```

## Project Structure

```text
plugins/admin-system/      One plugin (the first). Add more siblings under plugins/.
├── src/
│   ├── AMBuilder          This plugin's sources + link libs (discovered by root AMBuildScript)
│   ├── Core/              Plugin entry (derives CS2Kit::MetamodPluginBase), ChatService, Config
│   ├── Admin/
│   │   ├── AdminManager   Permissions, immunity, self-target allowed
│   │   ├── AdminMenu      Top-level dispatcher (Punish / Control / Effects)
│   │   ├── Actions/       Data-driven one-shots (Action/ParamAction descriptors + Run dispatch): Movement, Vitals, Launch, Teleport, Team, Smite, CheatCheck
│   │   ├── Effects/       Stateful per-target with Cancel (EffectToggle descriptors + Run): EffectManager, Ghost, Disco, Hide
│   │   └── Menu/          Per-category builders + PlayerPicker + PresetSubmenu
│   ├── Punishments/       PunishmentManager
│   ├── Commands/          Chat-command registrations
│   └── Database/          PostgreSQL pool, Entities/, Repositories/
├── configs/               settings.jsonc + migrations/ + translations/{en,ru}.json
├── database/              seed-admin.sql (optional manual superadmin seed)
└── admin-system.vdf       Metamod plugin registration
AMBuildScript              Root build: resolve SDK/MMS once, discover plugins/*, build each
configure.py               AMBuild entry point
vcpkg.json                 Root C++ deps (union across all plugins)
scripts/                   build.sh / deploy.sh / bootstrap.sh (operate on all plugins)
vendor/cs2-kit/            Reusable C++23 library (the only submodule). See its own CLAUDE.md.
references/                Read-only reference plugins — do NOT modify.
```

## CS2Kit Integration

Source inclusion (see Project Overview): all SDK dependencies live inside cs2-kit's `vendor/`, so admin-system has no duplicate submodules. Include style: `#include <CS2Kit/Commands/Command.hpp>`.

### Initialization Flow

`AdminSystemPlugin` (Plugin.cpp) derives from `CS2Kit::Core::MetamodPluginBase`, which owns the ISmmPlugin boilerplate, the four standard SourceHook hooks (GameFrame, client connect/disconnect, chat dispatch), the `PlayerManager` add/remove lifecycle, and `CS2Kit::Initialize`/`Shutdown`. The plugin only provides:

- `Info()` — plugin metadata (name, version, log tag).
- `OnLoad(late)` — subsystem wiring (configs, DB+admins, commands, punishments, translations, game-event listeners), formerly the separate Bootstrap module. Returns false on fatal config error. Cleanup is registered via `Defer()` and runs LIFO on unload/failed load.
- `OnPlayerConnect` — ban-on-connect kick. `OnPlayerDisconnect` — cancel effects. `OnPlayerChat` — `ChatService::HandleSay`.
- `OnRegisterHooks` — the one custom hook, `SetClientListening` (voice-mute suppression).

Config loads via `CS2Kit::Utils::Json::TryDeserializeFile<Settings>` — the `Settings` struct (in Config.hpp) mirrors settings.jsonc and auto-deserializes via nlohmann macros. The loader tolerates JSONC comments. A missing/unparseable settings.jsonc (or a wrong-typed value) is fatal; missing keys keep their defaults. Use `!admin_reload` to pick up DB changes.

## Code Conventions

### Naming (C# style)

| Element | Convention | Example |
| ------- | ---------- | ------- |
| Namespaces | `PascalCase`, nested | `AdminSystem::Players` |
| Classes/Structs | `PascalCase` | `PlayerManager`, `MenuOption` |
| Methods | `PascalCase` | `GetPlayer()`, `HandleInput()` |
| Member variables | `_camelCase` | `_admins`, `_states` |
| Constants | `PascalCase` | `MaxPlayers`, `InputDebounceMs` |
| Parameters/locals | `camelCase` | `steamId`, `targetSlot` |
| JSON keys | `camelCase` | `steamId`, `adminPanel` |

### C++ Style

- **C++17 nested namespaces:** `namespace AdminSystem::Sdk { ... }`
- **`.hpp` headers** (not `.h`)
- **Service container + accessor** — cs2-kit services via `Kit()` (`CS2Kit::Core::Services`); plugin managers in a `Managers` struct via `Sys()`. Built in `OnLoad`, destroyed on unload — no process-lifetime singletons.
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

- **`plugins/admin-system/configs/settings.jsonc`**: Plugin/database/punishments/chat/cheat-check configuration (JSONC — comments allowed). All non-admin runtime knobs live here.
- **`plugins/admin-system/configs/migrations/`**: Versioned SQL migrations (`NNNN_*.sql`) applied automatically on plugin load by the migration runner; the schema owns `admin_groups` and `admins`. Groups and individual admins are managed in PostgreSQL -- no JSON admin file. Use `!admin_reload` to pick up DB changes without a restart. `plugins/admin-system/database/seed-admin.sql` is an optional manual superadmin seed. To change the schema, add the next `configs/migrations/NNNN_*.sql` (forward-only).

## Database

- PostgreSQL; tables live in the default `public` schema
- Tables: admins, admin_groups, players, bans, voice_mutes, text_mutes, warnings, schema_migrations
- Prepared statements for all queries
- Mutex only in Database class (future async support)
