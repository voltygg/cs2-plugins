# Admin System - CS2 Metamod Plugin

## Project Overview

C++23 Metamod:Source plugin monorepo for CS2 community servers. Reusable engine
abstractions live in [cs2-kit](https://github.com/voltygg/cs2-kit), consumed as
the `cs2-kit` Conan package (`find_package(cs2-kit CONFIG REQUIRED)` ->
`CS2Kit::CS2Kit`). cs2-kit, hl2sdk-cs2 and metamod-source all arrive prebuilt
from the public voltygg Cloudsmith remote; `conanfile.py` pins the version.

`vendor/cs2-kit` is a submodule of the same repo, there so one IDE covers both. It
is inert until `conan editable add vendor/cs2-kit`, and `ignore = all` keeps its
pointer out of this repo's status.

Each plugin lives under `plugins/<name>/` with its own `src/`, `configs/`,
and `CMakeLists.txt` (`cs2_add_plugin(<name> ...)` + a root
`add_subdirectory()`; the .vdf is generated at install time). Scaffold a new
plugin with `uv run poe new-plugin <name>`.

Third-party C++ deps: add to `conanfile.py`, `find_package` in the root
`CMakeLists.txt`, link the imported target (e.g. `libpqxx::pqxx`) in the
plugin's `CMakeLists.txt`.

## Tech Stack

- Language: C++23
- Framework: Metamod:Source 2.0 + hl2sdk-cs2
- Build: CMake 4.3.4+ presets + Conan 2.29.1+
- Shared library: `CS2Kit::CS2Kit`
- Database: PostgreSQL via libpqxx
- UI: WASD center-HTML menus

## Build Commands

```bash
uv run poe build
uv run poe build windows-msvc-debug
uv run poe build-linux
ctest --preset windows-msvc-release
```

Build output lands in `build/<preset>/plugins/<name>/<platform-arch>/`.
Conan profiles and the remote are installed once with
`conan config install https://github.com/voltygg/cs2-kit.git -sf conan`
(`uv run poe bootstrap` does it). Builds precompile `<CS2Kit/Api.hpp>` per plugin
(extend with `cs2_add_plugin(... PCH_HEADERS ...)`, disable with
`-DCS2KIT_DISABLE_PCH=ON`); `cs2kit build --no-test` skips the ctest step (CI runs
tests separately). Unit tests use doctest (`test_requires` in conanfile.py) and
live in `plugins/<name>/tests/`, wired by the kit's `cs2_add_tests()`. One ctest
entry per case, so TEST_CASE names must not contain `[`, `]` or `;` (configure
enforces it).

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
plugins/anticheat/
  CMakeLists.txt
  src/
    Core/
    Correlation/
    Detectors/
    Response/
    Simulator/
  configs/
  tests/
plugins/bhop/
  CMakeLists.txt
  src/
  configs/
plugins/contracts/
CMakeLists.txt
CMakePresets.json
conanfile.py
conan.lock
scripts/
```

The build plumbing lives in the kit: `cs2_add_plugin`, `cs2_add_tests` and the
.vdf/manifest templates reach this repo as CMakeDeps build modules, so they are
available right after `find_package(cs2-kit CONFIG REQUIRED)`. The
`build`/`bootstrap`/`format`/`new-plugin` poe tasks are the `cs2kit-*` console
scripts from the `cs2-kit` Python distribution, which `pyproject.toml` depends on
and which also pins CMake/Conan/Ninja/clang-format. Local `scripts/` holds server
tooling; all deploy commands (remote and local) live in the `deploy/tools`
package (`python -m deploy.tools.cli`).

To work on the kit and a plugin together: `conan editable add <cs2-kit checkout>`
(see CONTRIBUTING.md).

## CS2Kit Integration

The SDKs are Conan packages behind cs2-kit, which re-exports them transitively.
Include style: `#include <CS2Kit/Commands/CommandSpec.hpp>` (or just
`<CS2Kit/Api.hpp>` for the hoisted short names).

Each plugin derives from `CS2Kit::MetamodPlugin`, which owns the ISmmPlugin
boilerplate, the standard SourceHook hooks and the PlayerManager lifecycle, and
creates the `CS2Kit::Runtime` for one load cycle. `OnLoad(Runtime&, bool late)`
receives it; each plugin builds its own `App` struct there (`src/App.hpp`, or
`src/Core/App.hpp` in admin-system) and drops it in `OnUnload`, so nothing
survives a `meta reload`. `CS2KIT_PLUGIN(Klass)` in Plugin.cpp expands the
instance and PLUGIN_EXPOSE; `CS2Kit::WithBuildInfo` stamps Info(), and
`CS2Kit::LoadStandardConfig` is the OnLoad config/translations prelude.
Plugin-domain rules (permissions, immunity, replies, broadcasts) are injected
once via `Runtime::Policy`. DB-using TUs include `<CS2Kit/Database/Api.hpp>` for
the short DB names - the main `Api.hpp` umbrella deliberately excludes pqxx.

Key kit patterns in use here:

- Commands are declarative `CommandSpec`s. Each `src/Commands/*.cpp` exposes a
  `RegisterXCommands(CommandManager&, App&)` that `App::Start()` calls, so a
  handler is handed what it needs instead of reaching for it. Typed args
  (Target/Duration/ReasonTail) are resolved before handlers run.
- Fun effects are `EffectDescriptor`s in `src/Admin/Effects/`, listed in the
  explicit `MenuEffects` table and rendered by menu context rows;
  punish/unmute/unban wizards are `Flow<TState>` chains in `src/Admin/Menu/`.
- Listener registrations return a `CS2Kit::Subscription`; hold it as a member
  next to whatever the callback captures rather than unregistering by hand.
- Repositories declare entity column tables (`Table`/`Key`/`Columns()`) and use
  the kit's `FromRow`/`InsertSql`/`SelectSql` mapping; hand-written SQL remains
  only for bespoke UPDATE/WHERE clauses.
- All player-facing text goes through `Runtime::Messages` and translation keys.

## Reference Projects

Local, git-ignored copies of third-party CS2 projects live in `references/`
(`CS2Fixes/`, `swiftlys2/`, and other repos) for consulting SDK usage and
implementation patterns. They are read-only references, not build inputs - do
not edit them or add them to the build.

## Code Conventions

- C++23.
- `.hpp` headers, not `.h`.
- C#-style naming: `PascalCase` types/methods, `_camelCase` members,
  `camelCase` locals/params, `PascalCase` constants.
- Use `std::format`, designated initializers, and `int64_t` for SteamIDs.
- Main-thread only. Do not add threads or mutexes in game code; the only
  threads/mutexes live inside the kit's `PostgresDatabase` worker and
  `HttpClient`, both of which replay completions on the game thread.
- Database access is async-first: `Query`/`Exec` with cache-first managers
  during gameplay; `QueryBlocking`/`WithConnection` only at load time
  (migrations, admin loads, `!admin_reload`).
- No singletons and no ambient lookups in plugin code: the runtime arrives in
  `OnLoad`, and each manager takes the collaborators it uses through its
  constructor. Free functions that are leaves of the graph (menu builders,
  command bodies) may take the whole `App&`.
- Prefer the umbrella short names (`CS2Kit::Type` via `#include <CS2Kit/Api.hpp>`)
  over `CS2Kit::Module::Type`. In `.hpp` never use a namespace-scope
  using-directive; `using namespace CS2Kit::X;` is allowed only in `.cpp` (TU-local).
- Keep source files around 300-350 LOC when practical.
- Comments are rare and should explain non-obvious reasons, not restate code.

## Config And Database

- Runtime config: `plugins/admin-system/configs/settings.jsonc`.
- SQL migrations: `plugins/admin-system/configs/migrations/`.
- Optional manual seed: `plugins/admin-system/database/seed-admin.sql`.
- Admin groups and admins live in PostgreSQL tables; use `!admin_reload` to
  refresh in-memory state after DB edits.
- Multiple servers share one database. `server.tag` in settings.jsonc is the
  server's stable identity; per-server group grants live in
  `admin_server_groups` keyed by that tag, while `admins.groups` stays global.
  Abuse-protection freeze state lives on the `admins` row (`is_frozen` etc.)
  and admin actions are audited in `admin_activity`.
