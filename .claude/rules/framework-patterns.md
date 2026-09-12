---
paths:
  - "plugins/**"
---

# Framework patterns

## Lifecycle

- Derive from `VoltMod::MetamodPlugin` and declare with `VOLTMOD_PLUGIN(Klass)` in `Plugin.cpp`.
- Build the object graph in `OnLoad(Runtime&)` and release it in `OnUnload`. The base creates a fresh `Runtime` per load cycle, so nothing may survive `meta reload`.
- Use `VoltMod::WithBuildInfo` for metadata and `VoltMod::LoadStandardConfig` for the config and translation load stages.

## Commands

Register from `App::Start()` with the fluent builder:

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

- Subscribe with `+=` on `Event` members (`runtime.Slots.Changed`, `runtime.Hooks.Movement.Pre`, ...) and with `runtime.GameEvents.On<T>()` for game events. A game event needs a struct in `Events/EventTypes.hpp`; there is no string form.
- Every subscription returns a `Subscription`. Keep it in a `Subscriptions` beside the state its handler captures: `_subs.Add(event += handler)`.
- Dropping a subscription unsubscribes, and cancels a `Scheduler` timer, so a fire-and-forget deferral still needs an owner.
- Hook services arm on the first subscription and disarm on the last. There is no `Install()`/`Enable()`. A leaked subscription leaves a live vtable hook after reload.
- For an engine vfunc the framework does not cover: `HookInterface` or `HookVTable` from `<VoltMod/Unsafe/Hook.hpp>`, keeping the `Subscription` it returns. A handler is a lambda taking the hooked object first; a pre returns `HookResult` or nothing, a post observes.

## Entities

- `Pawn` is the body (health, armor, movement, aim); `Controller` is the identity (name, money, team). Get them from `runtime.Entities.PawnOf(slot)` and `.Controller(slot)`.
- Both are frame-local. Store an `EntityRef` or `PlayerRef` and resolve again where used. `explicit operator bool()` is the only validity check.
- Schema fields are generated pairs: `pawn.Health()` reads, `pawn.SetHealth(100)` writes and replicates. Offsets are baked by `voltmod schemagen`; the load aborts if they no longer match the engine.
- `runtime.Ui.Panel(name)` returns a move-only `UiPanel` whose destructor removes the entity; hold it as a member. `runtime.Addons.Require(id)` returns a `Subscription`; the requirement lasts as long as you hold it.

## Errors and messages

- Return `Result<T>`/`Status` when the caller needs to know why. `Error::Detail` is log text, `Error::Key` the translation key for the player reply.
- All player-facing text goes through `Runtime::Messages` with translation keys.

## Menus and effects

- `MenuBuilder(title).Add(ButtonRow{...})` for rows, `ActionRows` for rows acting on an admin/target pair, `Flow<TState>::Create(menus, slot, state)` for multi-step actions.
- Open menus through `App::OpenMenu`; build rows against `App::MenuFor(slot)`, the `VoltMod::MenuSurface` that is the Panorama screen when the player can see it and `runtime.Menus` (center HTML) otherwise.
- Admin effects are `EffectDescriptor` values; menu order comes from the explicit `MenuEffects` table.

## Configuration

- A settings struct is a plain aggregate at namespace scope. The member name is the JSON key, a missing key keeps the initializer, unknown keys are ignored.
- When settings need validation or derived values, wrap `Json::ReadFile` in a plugin `ConfigManager` that publishes a snapshot in one assignment (admin-system `Config/ConfigManager.*`). Do not subclass `JsonConfig`: a failed reload must not leave half-applied state.
- Resolve `VoltMod::ConVar<T>` handles once at start, not by name per call.
- Ask `runtime.Capabilities.Has(...)` before using anything that depends on gamedata or an engine interface.

## Database

- Repositories use `Table`, `Key`, and `Columns()`. Keep handwritten SQL for specific UPDATE/WHERE clauses.
- admin-system settings: `configs/settings.jsonc`; migrations: `configs/migrations/`. Several servers may share one database and `server.tag` identifies each. Run `!admin_reload` after editing admin data by hand.
