# Admin System - CS2 Metamod Plugin

## Project Overview

C++23 Metamod:Source plugin monorepo for CS2 community servers. Reusable engine
abstractions live in `vendor/cs2-kit/`, which is consumed as the
`CS2Kit::CS2Kit` CMake target.

Each plugin lives under `plugins/<name>/` with its own `src/`, `configs/`,
`<name>.vdf`, and `CMakeLists.txt`. Add new plugins with
`cs2_add_plugin(<name> ...)` and a root `add_subdirectory()`.

## Tech Stack

- Language: C++23
- Framework: Metamod:Source 2.0 + hl2sdk-cs2
- Build: CMake 4.3.4+ presets + Conan 2.29.1+
- Shared library: `CS2Kit::CS2Kit`
- Database: PostgreSQL via libpqxx
- UI: WASD center-HTML menus

## Build Commands

```bash
scripts/build.sh
scripts/build.sh windows-msvc-debug
scripts/build.sh linux-steamrt-release
ctest --preset windows-msvc-release
```

Build output lands in `build/<preset>/plugins/<name>/<platform-arch>/`.

## Project Structure

```text
plugins/admin-system/
  CMakeLists.txt
  src/
    Core/
    Admin/
    Punishments/
    Commands/
    Database/
  configs/
  database/
  admin-system.vdf
CMakeLists.txt
CMakePresets.json
conanfile.py
cmake/
scripts/
vendor/cs2-kit/
```

## CS2Kit Integration

All SDK dependencies live inside cs2-kit's `vendor/`, so admin-system has no
duplicate SDK submodules. Include style: `#include <CS2Kit/Commands/Command.hpp>`.

`AdminSystemPlugin` derives from `CS2Kit::Core::MetamodPluginBase`, which owns
the ISmmPlugin boilerplate, standard SourceHook hooks, the PlayerManager
lifecycle, and `CS2Kit::Initialize` / `Shutdown`.

## Code Conventions

- C++23.
- `.hpp` headers, not `.h`.
- C#-style naming: `PascalCase` types/methods, `_camelCase` members,
  `camelCase` locals/params, `PascalCase` constants.
- Use `std::format`, designated initializers, and `int64_t` for SteamIDs.
- Main-thread only. Do not add threads or mutexes in game code; the only mutex
  is inside `Database`.
- Services/managers, not singletons. Use `Engine()` for cs2-kit services and
  `App()` for plugin managers.
- Keep source files around 300-350 LOC when practical.
- Comments are rare and should explain non-obvious reasons, not restate code.

## Config And Database

- Runtime config: `plugins/admin-system/configs/settings.jsonc`.
- SQL migrations: `plugins/admin-system/configs/migrations/`.
- Optional manual seed: `plugins/admin-system/database/seed-admin.sql`.
- Admin groups and admins live in PostgreSQL tables; use `!admin_reload` to
  refresh in-memory state after DB edits.
