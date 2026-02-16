# Admin System - CS2 Metamod Plugin

## Project Overview

C++23 Metamod:Source plugin for CS2 community servers providing admin functionality: player management, punishments (ban/mute/gag/warn), and WASD center-HTML menus.

## Tech Stack

- **Language:** C++23
- **Framework:** Metamod:Source 2.0 + hl2sdk-cs2
- **Database:** PostgreSQL 18 via libpqxx
- **UI:** WASD center-HTML menus (W/S navigate, E select, R close)
- **Build System:** AMBuild (auto-discovers .cpp files from `src/`)

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
│   └── Singleton.hpp           # CRTP singleton base (header-only)
├── Sdk/
│   ├── Entity.hpp/cpp          # Entity system access
│   ├── Schema.hpp/cpp          # Runtime schema field offset resolution
│   ├── UserMessage.hpp/cpp     # Chat & center HTML message sending
│   ├── SigScanner.hpp/cpp      # Signature scanning (byte pattern matching)
│   ├── GameData.hpp/cpp        # Centralized signature/offset management
│   └── GameInterfaces.hpp      # All SDK interface pointers (header-only)
├── Players/
│   ├── Player.hpp/cpp          # Player data (slot, SteamID, name, flags)
│   └── PlayerManager.hpp/cpp   # Player lifecycle tracking
├── Admin/
│   ├── AdminManager.hpp/cpp    # Permissions, flags (bitmask), immunity
│   └── AdminMenus.hpp/cpp      # Admin-specific menu builders
├── Commands/
│   ├── Command.hpp/cpp         # Command struct + CommandBuilder (fluent API)
│   └── CommandManager.hpp/cpp  # Registration, prefix handling, dispatch
├── Menu/                       # Generic reusable menu framework
│   ├── Menu.hpp                # Data structures + MenuLayout (header/footer)
│   ├── MenuBuilder.hpp         # Fluent builder pattern (header-only)
│   ├── MenuManager.hpp/cpp     # Lifecycle, WASD input, menu stack
│   └── MenuRenderer.hpp/cpp    # 3-section HTML rendering
├── Punishments/
│   └── PunishmentManager.hpp/cpp
├── Database/
│   ├── Database.hpp/cpp        # PostgreSQL connection pool
│   ├── Entities/               # Admin, AdminGroup, Ban, Mute, Gag, Warning, Player
│   └── Repositories/           # AdminRepository, BanRepository
└── Utils/
    ├── StringUtils.hpp/cpp     # String manipulation, target parsing
    ├── SteamId.hpp/cpp         # SteamID conversions
    ├── TimeUtils.hpp/cpp       # Time formatting
    └── Translations.hpp/cpp    # JSON-based i18n

configs/
├── settings.json               # Plugin, database, commands, punishments config
├── admins.json                 # Groups + admins (merged)
└── translations/               # en.json, ru.json
gamedata/
└── signatures.json             # Engine signatures and offsets
vendor/                         # Git submodules (hl2sdk-cs2, mmsource-2.0)
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
- **CRTP Singleton** (`Core::Singleton<T>`) for all singletons
- **No mutexes** (main-thread-only design; all Metamod hooks run on game thread)
- **Designated initializers** for struct construction
- **`std::format`** for string formatting
- **`int64_t`** for SteamIDs
- **`uint32_t` bitmask** for admin flags (a=bit0, b=bit1, ..., z=bit25)
- **Builder pattern** for Menu and Command construction

## Key Design Patterns

### Singleton (CRTP)

```cpp
class MyManager : public Core::Singleton<MyManager> {
    friend class Core::Singleton<MyManager>;
private:
    MyManager() = default;
};
```

### MenuBuilder (fluent)

```cpp
auto menu = Menu::MenuBuilder("Title")
    .AddItem("Option", [](int slot) { ... })
    .AddSubmenu("Sub", [](int slot) { return BuildSubmenu(slot); })
    .WithHeader([]() { return "<b>Custom Header</b>"; })
    .Build();
Menu::MenuManager::Instance().OpenMenu(slot, menu);
```

### CommandBuilder (fluent)

```cpp
cmdMgr.Register(
    Commands::CommandBuilder("kick")
        .WithAliases({"k"})
        .RequirePermission("c")
        .WithArgs(1, 2)
        .OnExecute([](Player* p, auto args) -> CommandResult { ... })
        .Build()
);
```

### SDK Abstractions

- **`GameInterfaces`**: Centralized holder for all SDK interface pointers (replaces scattered `extern` globals)
- **`GameData`**: Loads `signatures.json`, provides `FindSignature()` / `ResolveSignature()` (replaces duplicate gamedata parsing)

## Config Files

- **`settings.json`**: All plugin configuration (plugin, database, commands, punishments, admin sections)
- **`admins.json`**: Admin groups and admin entries (merged file)
- **`signatures.json`**: Engine signatures/offsets with platform-specific pattern+offset pairs

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
