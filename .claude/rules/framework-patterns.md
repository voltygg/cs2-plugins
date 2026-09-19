---
paths:
  - "plugins/**"
---

# Framework patterns

## Lifecycle

The plugin is a DLL the VoltMod host loads; the host is the only Metamod plugin. A plugin's
engine events arrive in load order, and a console command one plugin consumes is not offered to
the next.

- `plugin.json` beside the plugin's `CMakeLists.txt` is its identity: name, version, log tag, description, dependencies. Nothing in C++ or CMake repeats it; read `runtime.PluginName` and `runtime.Version`.
- Derive the load-cycle class from `VoltMod::Plugin`, construct the base from `Runtime&`, and override `bool Load()`. Put `VOLTMOD_PLUGIN(<Namespace>::App)` at global scope in `App.cpp`, with `<VoltMod/App/PluginEntry.hpp>` included in that one .cpp only. Override lifecycle hooks on `Plugin`; keep custom engine-hook subscriptions in the App's `_subs`.
- The `App` lives for one load cycle. Nothing may survive `volt reload`.
- `VoltMod::LoadStandardConfig(runtime, config)` loads settings and translations; `runtime.PluginFile("configs/x")` builds any other path under the plugin's directory.

## Commands

Register from `App::Load()` with the fluent builder:

```cpp
commands.Add("slap")
    .Describe("Slap a player")
    .Permission(Flag(Permission::Slap))
    .Run([&app](Caller c, Args::Target target, Args::Rest reason) -> Result<Reply> { ... });
```

- The handler's parameter list is the argument spec: `Caller` first, then one `Args::` value per argument. Targets, durations, and reasons arrive parsed and immunity-checked.
- `CommandManager` owns commands for the load cycle. Event, timer, and hook subscriptions live in the App's `_subs`.

## Authorization

- Inject permissions, immunity, replies, and broadcasts once through `Runtime::Policy`.
- Ask `Policy::Authorize(caller, target, permission)` wherever the answer is needed. Never re-implement the check.
- A plugin's `CanTarget` is an immunity comparison only; console and self-targeting are settled before it runs.

## Players

- Store a `PlayerRef` (slot + SteamID) and resolve it with `runtime.Players.Get(ref)` where used.
- Handle connect and disconnect by subscribing to `runtime.Players.Connected`, `.FullyConnected`, `.SettingsChanged`, `.Disconnected`. There are no lifecycle virtuals.

## Subscriptions and hooks

- Subscribe with `+=` on `Event` members (`runtime.Slots.Changed`, `runtime.Hooks.Movement.Before`, ...) and with `runtime.GameEvents.On<T>()` for game events. A game event needs a struct in `Events/EventTypes.hpp`; there is no string form.
- Every subscription returns a `Subscription`. Keep it in a `Subscriptions` beside the state its handler captures: `_subs.Add(event += handler)`.
- Dropping a subscription unsubscribes, and cancels a `Scheduler` timer, so a fire-and-forget deferral still needs an owner.
- Hook services arm on the first subscription and disarm on the last. There is no `Install()`/`Enable()`. A leaked subscription leaves a live vtable hook after reload, and the host logs it by name when the plugin unloads.
- For an engine function the framework does not cover: `HookInterface`, `HookVirtual` or `HookFunction` from `<VoltMod/Unsafe/Hook.hpp>`, keeping the `Subscription` it returns. A handler is a lambda taking the hooked object first; a before-handler returns `HookResult` or nothing, an after-handler observes.

## Entities

- `Pawn` is the body (health, armor, movement, aim); `Controller` is the identity (name, money, team). Get them from `runtime.Entities.PawnOf(slot)` and `.Controller(slot)`.
- Both are frame-local. Store an `EntityRef` or `PlayerRef` and resolve again where used. `explicit operator bool()` is the only validity check.
- Schema fields are generated pairs: `pawn.Health()` reads, `pawn.SetHealth(100)` writes and replicates. Offsets are baked by `voltmod schemagen`; the load aborts if they no longer match the engine.
- `runtime.Screens.Shared(layout)` and `runtime.Screens.ForPlayer(layout, slot)` return a move-only `Screen` whose destructor removes the entity; hold it as a member. `VoltMod::PlayerScreens` keeps one player screen per slot, created on first draw. `runtime.Addons.Require(id)` returns a `Subscription`; the requirement lasts as long as you hold it.

## Errors and messages

- Return `Result<T>`/`Status` when the caller needs to know why. `Error::Detail` is log text, `Error::Key` the translation key for the player reply.
- All player-facing text goes through `Runtime::Messages` with translation keys.

## Menus and effects

- `MenuBuilder(title).Add(ButtonRow{...})` for rows, `ActionRows` for rows acting on an admin/target pair, `Flow<TState>::Create(menus, slot, state)` for multi-step actions.
- Menus go through `runtime.Menus`: `Start` begins a session, `Open` pushes onto it. With Panorama on, a plugin prefers a `VoltMod::PanoramaMenu` on a `VoltMod::PanoramaMenuLayout`; a player without the layout gets center HTML.
- A Panorama menu screen is the framework's `menu` block styled by main-menu's `meatgg/theme.css.j2` and `meatgg/menu.css.j2`; don't restyle it per plugin.
- A plugin adds a main menu entry by publishing `Contracts::IMenuSection` with `Exchange.Publish<Contracts::IMenuSection>(impl, id)`; main-menu's config names the id.
- Admin effects are `EffectDescriptor` values; menu order comes from the explicit `MenuEffects` table.

## Configuration

- A settings struct is a plain aggregate at namespace scope. The member name is the JSON key, a missing key keeps the initializer, unknown keys are ignored.
- `VoltMod::Options<Settings>` loads the struct from `configs/settings.jsonc` and republishes it on every `Load`; bhop and anticheat use it as their `ConfigManager` directly.
- When settings need validation or derived values, give `Options` a snapshot type and the function that builds it (`Options<Settings, ConfigSnapshot>{&BuildSnapshot}`), and wrap it in a plugin `ConfigManager` that offers `Get()` plus the derived values (admin-system `Config/ConfigManager.*`); read a section as `config.Get().chat`, not through a getter per section. The builder runs on a local copy and the snapshot is published in one move, so a failed reload leaves the previous one intact. Never publish a half-validated value.
- Resolve `VoltMod::ConVar<T>` handles once at start, not by name per call.
- Ask the service's `Available()` (`runtime.Hooks.Movement`, `runtime.Screens`, ...) before relying on anything that depends on gamedata.

## Database

- `VoltMod::Database::RunAsync`/`Run` take a job callable over `auto& conn`, dispatched to whichever backend (Postgres, MariaDB, SQLite) is configured. A bare name blocks and is load-time only; `Async` returns first. Both report failure as `Result<T>` over `Error`.
- `VoltMod::Insert` returns the generated id; `VoltMod::Upsert` is the portable update-then-insert. Repository methods that return before the write lands end in `Async`.
- A migration is one dialect-free `configs/migrations/NNNN_name.sql`; `RunMigrations` substitutes `@ID@`, `@NOW@`, `@TRUE@`, `@FALSE@`, `@INSERT_IF_ABSENT@` and `@ON_CONFLICT(cols)@` for the live driver. Add a change as a new numbered file; never edit one that has been applied.
- Table specs are generated from those migrations by `uv run poe schema` into `src/Database/Tables/Schema.hpp`; `poe lint` fails if they drift. Never edit the generated header.
- Managers take `Database::Repositories&`, built once in `App`. Do not construct a repository at a call site.
- admin-system settings: `configs/settings.jsonc`; migrations: `configs/migrations/`. Several servers may share one database and `server.tag` identifies each. Run `!admin_reload` after editing admin data by hand.
