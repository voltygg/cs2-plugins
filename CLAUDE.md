# CS2 plugins repository

This repository builds C++23 Metamod:Source plugins for Counter-Strike 2. Shared
engine integration comes from the `voltmod/[~1.2]` Conan package. The local
`vendor/voltmod` checkout is a separate Git repository used for coordinated
framework work; it affects builds only after `conan editable add vendor/voltmod`.

## Commands

```bash
uv sync
uv run poe doctor
uv run poe bootstrap
uv run poe build
uv run poe test
uv run poe build windows-msvc-debug
uv run poe build-linux
uv run poe build --install <plugin> --start
ctest --preset windows-msvc-release
uv run poe lint
uv run poe format
```

`doctor` checks the local toolchain, project, Conan configuration, and an
optional CS2 server without changing them. `bootstrap` installs VoltMod's Conan
profiles and public remote, then builds. `build` compiles only; `test` brings
the build up to date and runs CTest. On `build`, `--install <plugin>` copies the
result into the local CS2 server at `CS2_SERVER_PATH` and `--start` launches
that server afterwards. All of these come from the framework CLI (`voltmod
build|test|install|serve`); `deploy/tools` only handles the remote fleet.
Build output is under
`build/<preset>/plugins/<name>/<platform-arch>/`. The build tasks run
`voltmod`, installed by this repository's `pyproject.toml`.

To work on VoltMod and a plugin together:

```bash
conan editable add vendor/voltmod
uv run poe build
conan editable remove voltmod
```

Treat the root worktree and `vendor/voltmod` as separate repositories. Check
their status and diffs independently. Do not edit the read-only projects under
`references/`.

## Repository map

```text
plugins/admin-system/  Admins, punishments, menus, reports, and PostgreSQL data
plugins/anticheat/     Detection cores, engine adapters, and response handling
plugins/bhop/          Server-wide and per-player bunnyhop modes
plugins/contracts/     Interfaces shared between plugin modules
deploy/                Docker deployment CLI, inventory, templates, and scripts
docs/                  Local development and deployment notes
vendor/voltmod/        Separate VoltMod framework checkout
```

Each plugin owns its `src/`, `configs/`, tests, and `CMakeLists.txt`.
`voltmod_add_plugin(<name> VERSION <version> ...)` discovers `src/*.cpp`, creates
the Metamod module, and generates its VDF and install bundle. Register a new
plugin with `add_subdirectory()` in the root `CMakeLists.txt`, or run:

```bash
uv run poe new-plugin <name>
```

Add third-party C++ dependencies to `conanfile.py`, find them in the root
`CMakeLists.txt`, and link their imported targets in the plugin CMake file.

## Plugin structure

Plugins derive from `VoltMod::MetamodPlugin`. The base creates one
`VoltMod::Runtime` per load cycle and passes it to
`OnLoad(Runtime&, bool late)`. Build the plugin's object graph there and release
it in `OnUnload`; no plugin state may survive `meta reload`.

Use `VOLTMOD_PLUGIN(Klass)` in `Plugin.cpp`. Use `VoltMod::WithBuildInfo` for
metadata and `VoltMod::LoadStandardConfig` for the normal configuration and
translation load stages.

The runtime owns framework services such as commands, players, menus, messages,
events, scheduling, HTTP, and engine wrappers. Database translation units must
include `<VoltMod/Database/Api.hpp>`; the main `<VoltMod/Api.hpp>` deliberately
does not include libpqxx.

Current patterns:

- Register declarative `CommandSpec` values from `App::Start()`. Resolve typed
  targets, durations, and reasons before handlers run.
- Inject permissions, immunity, replies, and broadcasts once through
  `Runtime::Policy`.
- Keep each returned `VoltMod::Subscription` beside the state captured by its
  callback.
- Define admin effects as `EffectDescriptor` values and keep menu order in the
  explicit `MenuEffects` table.
- Use `Flow<TState>` for multi-step menu actions.
- Define repository column tables with `Table`, `Key`, and `Columns()`. Keep
  handwritten SQL for queries whose UPDATE or WHERE clauses are specific.
- Send player-facing text through `Runtime::Messages` and translation keys.

## Conventions

- Use C++23 and `.hpp` headers.
- Use `PascalCase` for types and methods, `_camelCase` for members, and
  `camelCase` for local variables and parameters.
- Prefer `std::format`, designated initializers, and `int64_t` SteamIDs.
- Prefer the short names exported by `<VoltMod/Api.hpp>`. Do not put a
  namespace-scope using-directive in a header.
- Game code runs on the main thread. Do not add plugin-owned worker threads or
  mutexes. VoltMod's database and HTTP workers replay completions on the game
  thread.
- Use asynchronous database calls during gameplay. Blocking calls are limited
  to load-time work such as migrations and explicit admin reloads.
- Pass dependencies through constructors. Do not add singletons or ambient
  service lookups in plugin code.
- Bind stable services in constructors and App-owned objects named by behaviour
  (Actions, PlayerEffects, Runtime.Pawns); pass request data such as slots and
  descriptors to methods. Do not add generic Services/Env bags. `Runtime&` may
  reach plugin action handlers through `ActionContext::Rt` but never Sdk code.
- Keep files near 300-350 lines when that improves readability.

## Commenting and documentation

- Comment only to explain intent, constraints, ownership, lifetime, threading,
  security, or compatibility. Keep comments near the code and remove stale or
  obvious narration.
- Use Doxygen for non-obvious public contracts. Cover preconditions, ownership,
  errors, return behavior, and concurrency only when relevant; preserve exact
  symbols and tags.
- Keep docs task-first, accurate, and runnable. Preserve commands, paths,
  identifiers, defaults, links, and configuration keys.
- Use plain English and sentence-case headings. Call VoltMod the framework;
  reserve "library" for actual libraries or CMake targets. Update affected docs
  and templates with behavior or API changes.

## Tests and configuration

Plugin tests live under `plugins/<name>/tests/` and use doctest through
`voltmod_add_tests()`. Each doctest case becomes a CTest entry, so case names
must not contain `[`, `]`, or `;`.

The admin-system runtime settings are in
`plugins/admin-system/configs/settings.jsonc`; migrations are in its
`configs/migrations/` directory. Several servers may share the database.
`server.tag` is the stable per-server identity used by `admin_server_groups`.
Admin freeze state is stored on `admins`, and actions are audited in
`admin_activity`. Run `!admin_reload` after changing admin data directly.
