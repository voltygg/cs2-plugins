# CS2 plugins repository

## Project overview

C++23 Metamod:Source plugin repository for CS2 community servers. Shared engine
abstractions live in the [VoltMod framework](https://github.com/voltygg/voltmod),
consumed as the `voltmod` Conan package
(`find_package(voltmod CONFIG REQUIRED)` -> `VoltMod::VoltMod`). VoltMod,
`hl2sdk-cs2`, and `metamod-source` arrive prebuilt from the public volty
Cloudsmith remote; `conanfile.py` pins the version.

`vendor/voltmod` is a submodule so one IDE can cover both repositories. It is
inert until `conan editable add vendor/voltmod`, and `ignore = all` keeps its
pointer out of this repository's status.

Each plugin lives under `plugins/<name>/` with its own `src/`, `configs/`,
and `CMakeLists.txt` (`voltmod_add_plugin(<name> VERSION <version> ...)` + a root
`add_subdirectory()`; the .vdf is generated at install time). Scaffold a new
plugin with `uv run poe new-plugin <name>`.

Third-party C++ deps: add to `conanfile.py`, `find_package` in the root
`CMakeLists.txt`, link the imported target (e.g. `libpqxx::pqxx`) in the
plugin's `CMakeLists.txt`.

## Tech stack

- Language: C++23
- Framework: Metamod:Source 2.0 + hl2sdk-cs2
- Build: CMake 4.3.4+ presets + Conan 2.29.1+
- Framework target: `VoltMod::VoltMod`
- Database: PostgreSQL via libpqxx
- UI: WASD center-HTML menus

## Build commands

```bash
uv run poe build
uv run poe build windows-msvc-debug
uv run poe build-linux
ctest --preset windows-msvc-release
```

Build output is written to `build/<preset>/plugins/<name>/<platform-arch>/`.
Conan profiles and the remote are installed once with
`conan config install https://github.com/voltygg/voltmod.git -sf conan`
(`uv run poe bootstrap` does it). Builds precompile `<VoltMod/Api.hpp>` per plugin
(extend with `voltmod_add_plugin(... PCH_HEADERS ...)`, disable with
`-DVOLTMOD_DISABLE_PCH=ON`); `voltmod build --no-test` skips the ctest step (CI runs
tests separately). Unit tests use doctest (`test_requires` in conanfile.py) and
live in `plugins/<name>/tests/`, wired by the framework's `voltmod_add_tests()`. One ctest
entry per case, so TEST_CASE names must not contain `[`, `]` or `;` (configure
enforces it).

## Project structure

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

The build plumbing lives in the framework: `voltmod_add_plugin`,
`voltmod_add_tests`, and the `.vdf`/manifest templates reach this repository as
CMakeDeps build modules, so they are
available right after `find_package(voltmod CONFIG REQUIRED)`. The
`build`/`bootstrap`/`format`/`new-plugin` poe tasks are the `voltmod-*` console
scripts from the `voltmod` Python distribution, which `pyproject.toml` depends on
and which also pins CMake/Conan/Ninja/clang-format. Local `scripts/` holds server
tooling; all deploy commands (remote and local) live in the `deploy/tools`
package (`python -m deploy.tools.cli`).

To work on the framework and a plugin together, run
`conan editable add <voltmod checkout>` (see CONTRIBUTING.md).

## VoltMod integration

The SDKs are Conan packages behind voltmod, which re-exports them transitively.
Include style: `#include <VoltMod/Commands/CommandSpec.hpp>` (or just
`<VoltMod/Api.hpp>` for the hoisted short names).

Each plugin derives from `VoltMod::MetamodPlugin`, which owns the ISmmPlugin
boilerplate, the standard SourceHook hooks and the PlayerManager lifecycle, and
creates the `VoltMod::Runtime` for one load cycle. `OnLoad(Runtime&, bool late)`
receives it; each plugin builds its own `App` struct there (`src/App.hpp`, or
`src/Core/App.hpp` in admin-system) and drops it in `OnUnload`, so nothing
survives a `meta reload`. `VOLTMOD_PLUGIN(Klass)` in Plugin.cpp expands the
instance and PLUGIN_EXPOSE; `VoltMod::WithBuildInfo` stamps Info(), and
`VoltMod::LoadStandardConfig` is the OnLoad config/translations prelude.
Plugin-domain rules (permissions, immunity, replies, broadcasts) are injected
once via `Runtime::Policy`. DB-using TUs include `<VoltMod/Database/Api.hpp>` for
the short DB names - the main `Api.hpp` umbrella deliberately excludes pqxx.

Key framework patterns in use here:

- Commands are declarative `CommandSpec`s. Each `src/Commands/*.cpp` exposes a
  `RegisterXCommands(CommandManager&, App&)` that `App::Start()` calls, so a
  handler is handed what it needs instead of reaching for it. Typed args
  (Target/Duration/ReasonTail) are resolved before handlers run.
- Fun effects are `EffectDescriptor`s in `src/Admin/Effects/`, listed in the
  explicit `MenuEffects` table and rendered by menu context rows;
  punish/unmute/unban wizards are `Flow<TState>` chains in `src/Admin/Menu/`.
- Listener registrations return a `VoltMod::Subscription`; hold it as a member
  next to whatever the callback captures rather than unregistering by hand.
- Repositories declare entity column tables (`Table`/`Key`/`Columns()`) and use
  the framework's `FromRow`/`InsertSql`/`SelectSql` mapping; hand-written SQL remains
  only for bespoke UPDATE/WHERE clauses.
- All player-facing text goes through `Runtime::Messages` and translation keys.

## Reference projects

Local, git-ignored copies of third-party CS2 projects live in `references/`
(`CS2Fixes/`, `swiftlys2/`, and other repos) for consulting SDK usage and
implementation patterns. They are read-only references, not build inputs - do
not edit them or add them to the build.

## Code conventions

- C++23.
- `.hpp` headers, not `.h`.
- C#-style naming: `PascalCase` types/methods, `_camelCase` members,
  `camelCase` locals/params, `PascalCase` constants.
- Use `std::format`, designated initializers, and `int64_t` for SteamIDs.
- Main-thread only. Do not add threads or mutexes in game code; the only
  threads/mutexes live inside the framework's `PostgresDatabase` worker, `HttpClient`,
  and the deferred-log queue those two feed - all three replay on the game
  thread. Log from a worker with the normal `Log::` calls; `Core::Emit` queues
  the line rather than letting a worker reach tier0.
- Database access is async-first: `Query`/`Exec` with cache-first managers
  during gameplay; `QueryBlocking`/`WithConnection` only at load time
  (migrations, admin loads, `!admin_reload`).
- No singletons and no ambient lookups in plugin code: the runtime arrives in
  `OnLoad`, and each manager takes the collaborators it uses through its
  constructor. Free functions that are leaves of the graph (menu builders,
  command bodies) may take the whole `App&`.
- Prefer the umbrella short names (`VoltMod::Type` via `#include <VoltMod/Api.hpp>`)
  over `VoltMod::Module::Type`. In `.hpp` never use a namespace-scope
  using-directive; `using namespace VoltMod::X;` is allowed only in `.cpp` (TU-local).
- Keep source files around 300-350 LOC when practical.
- Comments are rare and should explain non-obvious reasons, not restate code.

## Config and database

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
