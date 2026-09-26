# VoltMod plugin API redesign

Status: planned 2026-09-26 from an API review against SwiftlyS2, CounterStrikeSharp and Plugify
(`references/`), then cut down to the simplest version of each change. Not started. Facts checked
against voltmod `cf7cdac` and cs2-plugins `1e86236`.

## Goal

Plugin code gets shorter and harder to get wrong. Each change is the smallest one that removes a
cost in today's code; anything without a user today waits in "On demand". When this plan is done:

- A plugin declares no entry macro and no constructor, and writes a `Load()` only when it needs one.
- Command handlers need no trailing return type, except the two that mix silence with a reply.
- A translated message is one call, and a message to everyone arrives in each player's language.
- `.Permission("x")` works in every plugin as soon as admin-system is loaded.
- Map start and chat are events like the other lifecycle signals. No plugin redoes menu input or `!`
  command dispatch.
- Publishing a cross-plugin service returns a `Subscription`, so nothing is withdrawn by hand.
- The runtime's services are ready when built, with no `Initialize()` or `Attach()` step.
- A plugin class that coordinates much of its App takes `App&` instead of a long argument list.
- Admin-only types live in admin-system, and `<VoltMod/Api.hpp>` shows only general tools.
- A banned player is refused at connect and sees the reason.
- `plugin.json` carries a website, a license and a starting log level, and editors complete it.
- Every game event has a generated struct, and no event handler sees an invalid slot for the player
  its event is about.

Rules for this change:

- The simplest version that works: no subsystem, option or overload without a user in this repo
  today. The one new tool, the event generator, is a short script that follows `schemagen`.
- Breaking changes are fine; the framework has no external consumers. Every caller is updated in the
  same change, with no aliases or shims.
- Keep what the review found better than the references: typed command arguments, frame-local
  wrappers with storable refs, generated setters that replicate, a `Subscription` for every
  registration, `PerSlot`, `Options` and `ServiceExchange`.
- Keep the `Runtime` and every `App` as flat member lists in dependency order. Members are destroyed
  in reverse, which is what makes a reload tear down cleanly. Inside the framework, constructors keep
  taking only the services they use, which lets its 475 test cases run without the SDK.
- One way to block: an event struct carries a `Blocked` flag, as `DamageHit` does. Only raw `Unsafe`
  hooks return `HookResult`.
- The rules files (`voltmod/.claude/rules/design.md`, `.claude/rules/framework-patterns.md`,
  `.claude/rules/cpp.md`) change in the same phase as the API they describe, because future sessions
  load them.
- Work on `feat/plugin-api-redesign` in every repo the change touches. Build against the editable
  `voltmod` checkout, and commit in groups, one per phase or topic, voltmod first.
- No release and no relock. A single `/deploy-test`, run once all phases are complete, not per phase.

## Facts this plan relies on

1. **Commands.** All 28 command handlers in `plugins/` declare `-> Result<Reply>`. `Caller::Ok`
   returns `Result<Reply>` but `Caller::Fail` returns `std::unexpected<Error>`
   (`voltmod/src/Commands/CommandBuilder.cpp:13-23`), so a lambda returning both cannot deduce its
   type. `Fail` is returned only from handlers and from admin-system's `Punish` helper, which already
   returns `Result<Reply>`.
   - Nine handlers end with `return Reply::Silent();`. `FreezeCommands.cpp:130` and
     `ReportCommand.cpp:40` mix it with `c.Ok`/`c.Fail`.
   - `t.Value->` appears 4 times (`CheatCheckCommands.cpp`), `x.Value ? x.Value->Value : y` twice
     (`PunishmentCommands.cpp:58`, anticheat `CommandDump.cpp:38`), and `c.Tr.Get` twice
     (`PunishmentCommands.cpp:91` and `:114`).
2. **Messages.** `Messages` has `Send`, `Broadcast`, `Reply` (which is `Send` to chat) and `ReplyKey`
   (chat only). `Broadcast` posts one string to every slot (`voltmod/src/Messaging/Messages.cpp:118`),
   so every broadcast is in the server language. Call sites:
   - voltmod: `CommandManager.cpp:97`.
   - admin-system: `FreezeManager.cpp:161`, `AdminActionsService.cpp:67`, `ChatService.cpp:68-141`,
     `PlayerChat.cpp:81`, `ReportFlow.cpp:85`, `ReportMenuSection.cpp:40`.
   - bhop: `BhopManager.cpp:148` (`Send` plus `Translations.Get`).
   - stronghold: `Hud.cpp:207-214`, where `NotifyAll` calls `ReplyKey` for every player in a loop.
3. **Permissions.** `Policy::CheckPermission` denies everything while `HasPermission` is unset
   (`voltmod/src/Players/Policy.cpp:14-27`), and only admin-system sets it.
   `contracts/include/Contracts/IPermissions.hpp` tells every other plugin to install its own lambda
   over `Exchange.Get<IPermissions>()`; none does. `PluginModule::AttachImpl` adds a `Permissions`
   load step that warns about commands gated on an unset policy (`CommandManager::CommandsMissingPolicy`).
4. **Lifecycle virtuals.** `Plugin` has `Load`, `OnServerStartup(map)` and `OnPlayerChat(...)`.
   - Overriding `OnPlayerChat` replaces menu text input and `!` dispatch
     (`voltmod/src/App/PluginChat.cpp`). admin-system's `PlayerChat::HandleSay`
     (`PlayerChat.cpp:84-103`) redoes both by hand.
   - anticheat's and stronghold's `OnServerStartup` overrides only forward to their members.
5. **Two-step framework start.** `PluginModule::AttachImpl` builds `Runtime(host.Languages())`,
   calls `Exchange.Attach` and `Commands.Attach`, then runs `Runtime::Initialize`
   (`voltmod/src/Runtime.cpp`), which:
   - installs the log handler;
   - fills `Unsafe.Interfaces` and `Unsafe.Bindings`, which every engine-backed member already
     holds references to;
   - checks the schema stamp;
   - runs `Messages.Initialize()` (required), then `Entities.Initialize()`, `ConVars.Initialize()`,
     `GameEvents.Initialize()` and `Hooks.ClientConVars.Initialize()` as optional load steps.

   `PluginName` and `Version` are public strings set there.
6. **Two-step App start.** The module constructs the App and then calls `Load`, and settings load
   inside `Load`.
   - Members therefore subscribe before config exists, and `BhopManager`, `Detectors` and
     `CheatSimulator` each grew an `Initialize()`.
   - Two reload paths repeat the settings path: `BhopManager.cpp:156` and anticheat
     `Engine/Commands.cpp:29`.
7. **Entry macro.** `VOLTMOD_PLUGIN` (`voltmod/include/VoltMod/App/PluginEntry.hpp`) differs between
   plugins only by the class name.
   - All five plugins declare `<PascalCase name>::App` in `src/App.hpp`, and the scaffold uses the
     same names (`voltmod/cli/voltmod/scaffold/scaffold.py`, `template_values`).
   - No test target compiles `App.cpp`.
   - Precedent: `voltmod/cmake/DoctestMain.cpp` is a framework-owned source compiled into every test
     target.
8. **App wiring.**
   - stronghold's `App` wires 31 members with 127 constructor arguments. Five of them need more than
     five of its members besides `Runtime`: `Panel` (13 arguments), `Store` and `Mover` (8 each),
     and `Placer` and `Health` (7 each).
   - admin-system already passes `App&` to its menus, menu sections and commands (14 headers), and
     declares `App` once in `Core/Types.hpp`. The plugin lint accepts one `*Types.hpp` per plugin
     (`voltmod/cli/voltmod/checks/conventions.py`).
   - admin-system and main-menu each assemble their Panorama menu from a
     `std::optional<PanoramaMenu>`, `Runtime.PanoramaMenuServices()` and a separate
     `PreferPanorama` subscription.
9. **Exchange.** `Publish` and `Unpublish` are a manual pair. admin-system's `~App` calls four
   `Unpublish`es (`App.cpp:29-38`) and stronghold's calls one. The host logs anything left published
   as a bug.
10. **Admin-only types.** `Action`, `ParamAction`, `ActionContext`, `ActionDispatcher`,
    `EffectDescriptor`, `EffectDispatcher`, `EffectManager` and `ActionRows` are used only by
    admin-system, which already aliases them into its own namespace
    (`plugins/admin-system/src/Admin/Actions/ActionContext.hpp`).
    - `<VoltMod/Api.hpp>` includes `Players/EffectDispatcher.hpp`, and `<VoltMod/Menu/Api.hpp>`
      includes `ActionRows.hpp`.
    - `Policy::Broadcast` is read only by `ActionDispatcher`.
    - `voltmod/tests/Players/EffectManagerTests.cpp` has 8 cases.
11. **Connect.** The host hooks `IServerGameClients::OnClientConnected` (`EngineHooks.cpp:126`), which
    runs after the player is already in. admin-system therefore kicks a banned player on the next tick
    (`App.cpp`, `OnPlayerConnect`; `PunishmentManager.cpp:260`). `ClientConnect(CPlayerSlot, const char*
    name, uint64 xuid, const char* networkId, bool, CBufferString* rejectReason)` returns a bool and
    carries a reject reason (`references/hl2sdk/public/eiface.h:569`).
12. **Host lists.** `volt list` prints only loaded plugins (`VoltCommand.cpp`, `PrintLoaded`), and
    `LoadList.Refused` is logged once and then dropped (`PluginLoader.cpp:108-113`).
13. **Manifest.** `plugin.json` has `name`, `version`, `logTag`, `description`, `author`,
    `dependencies`, `optionalDependencies` and `database`.
    - The host reads it strictly through Glaze (`src/Host/Loading/InstalledPlugins.cpp`,
      `PluginDocument`), so an unknown key refuses the plugin.
    - The CLI has no JSON-schema library; `gamedata/gamedata.schema.json` serves editors only.
    - stronghold's `description` and `author` are empty.
    - No plugin has a required dependency; anticheat lists admin-system as optional.
14. **Language.** A player's language changes only when they pick one in main-menu (`HubMenu.cpp:79`)
    or in stronghold's shop (`Shop.cpp:270`), although every plugin ships `en.json` and `ru.json`.
    `Hooks.ClientConVars.Query` can already ask a client for a convar.
15. **Events.** There are 14 hand-written structs in `Events/EventTypes.hpp`, and plugins use 10 of
    them. `EventTypes.cpp` decodes `userid` with `GetPlayerSlot`, which gives -1 for a missing player.
    - Plugins carry 50 `IsValidSlot` guards, most of them protecting `PerSlot` indexing.
    - `PlayerDeath` and `PlayerHurt` name their player `VictimSlot`; the others name it `Slot`.
    - `BulletImpact::Slot` is a best-effort decode of a truncated userid, which anticheat reads as a
      fallback (`DetectionFeed.cpp:376-379`).
    - The game defines 289 events in three KeyValues files inside its VPKs: `resource/core.gameevents`
      (94) in `game/core/pak01_dir.vpk`, and `resource/game.gameevents` (50) and
      `resource/mod.gameevents` (145) in `game/csgo/pak01_dir.vpk`. Read on the local server on
      2026-09-26.
    - Their field types are `short`, `string`, `player_controller`, `float`, `long`, `bool`, `byte`,
      `player_controller_and_pawn`, `player_pawn` (11), `uint64`, `int` and `ehandle` (3). Five
      events also carry a `local` flag, which is not a field.
    - The CLI has no VPK or KeyValues reader. It already renders `schemagen`'s output from Jinja
      templates.
16. **Cleanup.** admin-system's `~App` calls `Runtime.Players.Clear()` (`App.cpp:36`), which
    `PlayerManager.hpp` marks internal; the App's `OnPlayerDisconnect` is its only `Disconnected`
    handler. No plugin uses `Logger<T>` or `EntitySystem::AlivePawns()`.
17. **No chrono literals.** Plugin code bans `using namespace`, so `Scheduler.Repeat(1min, ...)` would
    have to be `Repeat(std::chrono::minutes{1}, ...)`, which reads no better than `60'000`.

## The result, in one plugin

bhop after phases 1 and 2:

```cpp
// src/App.hpp - the whole App; there is no App.cpp
struct App final : VoltMod::Plugin
{
    using Plugin::Plugin;

    ConfigManager Config = VoltMod::LoadConfig<ConfigManager>(Runtime);
    BhopManager Bhop{Runtime, Config};  // reads Config.Get() and registers its commands when built
};
```

```cpp
commands.Add("bhop_reload").ServerOnly().Run([this](Caller) { ReloadSettings(); });

_rt.Messages.SendKey(slot, enabled ? "bhop.granted" : "bhop.revoked", {}, VoltMod::MessageKind::Center);
```

## Phase 1 - Commands and messages

### Framework

1. `Commands/CommandBuilder.hpp`, `src/Commands/CommandBuilder.cpp`:
   - `Caller::Fail` returns `Result<Reply>`, so a handler returning `c.Ok` and `c.Fail` deduces its
     type.
   - `Bind` also accepts a handler that returns nothing, as a silent success.
   - `Reply::Silent()` stays for the two handlers that mix silence with `Ok`/`Fail`; they keep
     `-> Result<Reply>`.
   - Rename `Caller::Tr` to `Caller::Translations`; plugin code calls `c.Text(key)`.
2. `Commands/Args.hpp`, two additions and nothing else:
   - `Target::operator->`, so `t.Value->Name()` becomes `t->Name()`.
   - `Opt<T>::ValueOr(fallback)`, which returns the inner value, so
     `why.Value ? why.Value->Value : x` becomes `why.ValueOr(x)`.
3. `Messaging/Messages.hpp`, `src/Messaging/Messages.cpp`:
   - Add `SendKey(slot, key, tokens = {}, kind = Chat)`, which translates `key` into the slot's
     language and sends it.
   - Add `BroadcastKey(key, tokens = {}, kind = Chat)`, which calls `SendKey` for each slot with a
     client (the check `Broadcast` already uses for center HTML).
   - `Send` and `Broadcast` keep sending finished text, unchanged.
   - Delete `Reply` and `ReplyKey`. `CommandManager.cpp:97` calls `Send`.
4. Tests, in `tests/Commands/CommandBuilderTests.cpp`:
   - A handler returning nothing replies nothing.
   - A lambda returning `c.Ok` and `c.Fail` compiles without a trailing return type.
   - `Opt<Rest>::ValueOr` and `Target`'s `operator->`.

### Plugins

- Every handler loses `-> Result<Reply>`, except `FreezeCommands.cpp:130`'s and `ReportCommand.cpp:40`'s.
  The other silent handlers return nothing.
- `t.Value->` becomes `t->`, `why.Value ? why.Value->Value : x` becomes `why.ValueOr(x)`, and
  `c.Tr.Get(k)` becomes `c.Text(k)`.
- Messages:
  - `ReplyKey` becomes `SendKey` in `AdminActionsService.cpp`, `ReportFlow.cpp` and
    `ReportMenuSection.cpp`.
  - `Reply` becomes `Send` in `FreezeManager.cpp` and `ChatService.cpp`.
  - `Broadcast` in `ChatService.cpp` and `PlayerChat.cpp` stays as it is.
  - bhop's `Send(slot, Translations.Get(key, slot), Center)` becomes
    `SendKey(slot, key, {}, MessageKind::Center)`.
  - stronghold's `Hud::NotifyAll` calls `BroadcastKey` once instead of `ReplyKey` per player.
- admin-system's `ChatService` broadcasts stay one server-language line; they build composite lines.
- Template: `voltmod/templates/plugin/src/Commands.cpp`.

### Docs and rules

`voltmod/docs/commands.md`, `chat.md`, `sdk/messaging.md`, `players.md`, `config.md` and
`architecture.md`; the Commands and "Errors and messages" sections of `.claude/rules/framework-patterns.md`.

### Done when

- `uv run poe test` passes in both repos and `uv run poe lint` is clean.
- On the local server:
  - `!ping` answers.
  - `!kick` replies, with and without a reason.
  - `bhop_player <steamid> 1` shows the center notice in the player's chosen language.
  - With one player switched to `ru`, a stronghold `NotifyAll` message reaches each player in their
    own language.

## Phase 2 - Plugin and runtime shape

### 2.1 Entry point without a macro

- Add `voltmod/cmake/PluginEntry.cpp.in`. It holds the body of `VOLTMOD_PLUGIN` as plain code and
  includes `"App.hpp"`:
  - the `PluginModule` for `@plugin_class@`;
  - the plugin's own `KHook::__exported__khook`;
  - the descriptor and the exported `VoltMod_PluginEntry`.
- `voltmod_add_plugin` in `cmake/VoltModPlugin.cmake`:
  - Derives the namespace from the target name the way `template_values` does, by splitting on `-`
    and capitalizing each word: `admin-system` becomes `AdminSystem`.
  - Fails at configure time when `src/App.hpp` is missing, with the message
    `src/App.hpp must declare AdminSystem::App`.
  - Runs `configure_file` on the template into `${CMAKE_CURRENT_BINARY_DIR}/PluginEntry.cpp` and adds
    that file to the target.
- Delete `include/VoltMod/App/PluginEntry.hpp`. `MakePluginModule` and its `PluginType` concept stay
  in `App/Internal/PluginModule.hpp`, so a wrong class still fails to compile and names that concept.
- Check that the Conan package ships `cmake/PluginEntry.cpp.in`, as it ships `DoctestMain.cpp`.

### 2.2 The runtime is ready when built

- `PluginModule::AttachImpl` installs the log handler and checks the schema stamp first, then:

  ```cpp
  UnsafeServices unsafe = UnsafeServices::Open(host);  // interfaces and gamedata bindings, before any service
  _runtime = std::make_unique<Runtime>(host, unsafe);  // each service does its setup in its constructor
  _plugin = _factory(*_runtime);
  if (std::string reason = _runtime->LoadSteps.AbortReason(); !reason.empty())
      return Refuse(reason);
  ```

  It subscribes host events and calls `Load()` only after that check.
- `UnsafeServices::Open(IHost&)` resolves the interfaces and binds the gamedata. The module owns the
  result, and `Runtime::Unsafe` becomes a reference to it, so `Runtime.Unsafe.Interfaces.Engine`
  reads the same as today.
- Each engine-backed service does its setup in its constructor, and `Available()` keeps the reason
  when a piece did not bind:
  - `Messages` resolves its network messages;
  - `GameEvents` attaches to the event manager;
  - `ConVars` takes `ICvar`;
  - `ClientConVars` installs its response hook;
  - `EntitySystem` reads the entity system, and still reads it again at map start.
- The `Runtime` constructor records `Messages` as a required load step and the rest as optional ones,
  from their `Available()`, and logs unavailable features, as `InitializeServices` does today.
- `ServiceExchange` and `CommandManager` take the host in their constructors. `PluginName` and
  `Version` become `const`, set from the host.
- Deleted: `Runtime::Initialize`, `InitializeServices`, `LoadContext`, the five `Initialize()`
  methods and both `Attach()` methods. `ClientConVars::Shutdown` becomes part of its destructor.

### 2.3 The App is built in one step

- `Plugin::Load` becomes `virtual bool Load() { return true; }`.
- `LoadConfig` returns the config:
  `template <class TConfig> TConfig LoadConfig(Runtime&, TConfig config = {}, const LoadConfigOptions& = {})`.
  It still records the required `Configuration` step and loads translations. A failure leaves the
  defaults in place, and the check in 2.2 refuses the plugin before `Load` runs.
- `Options` remembers the path it loaded, and `Status Reload()` reads it again. admin-system's
  `Config::ConfigManager` forwards `Reload()`.
- Every App:
  - `using Plugin::Plugin;` replaces the constructor line.
  - `Config` is initialized through `LoadConfig`. main-menu writes
    `LoadConfig(Runtime, ConfigManager{&CleanSettings})`, stronghold passes
    `ConfigManager{&BuildSettings}` the same way, and anticheat passes `{.Translations = false}`.
- `BhopManager::Initialize`, `Detectors::Initialize` and `CheatSimulator::Initialize` fold into their
  constructors, and bhop's `App.cpp` goes away.
- bhop's `ReloadSettings` and `anticheat_reload` call `Reload()`.

### 2.4 Map start and chat are events

- `Engine/Server/Map.hpp` gets `Event<std::string_view> Started`, raised by
  `PluginModule::OnServerStartup` after the framework's own services have refreshed.
- `Players/PlayerManager.hpp` gets this struct, and `Event<ChatMessage&> Said` next to
  `Connected`/`Disconnected`:

  ```cpp
  /** A chat line that was neither menu input nor a command. Set Blocked to keep it out of chat. */
  struct ChatMessage
  {
      Player& Sender;
      std::string_view Text;
      bool TeamOnly = false;
      bool Blocked = false;
  };
  ```

- `src/App/PluginChat.cpp`: the module runs `ChatInput.TryConsume`, then `Commands.HandleChatMessage`,
  then raises `Said` and returns `Blocked`. Delete `Plugin::OnServerStartup` and `Plugin::OnPlayerChat`.
- admin-system drops its `OnPlayerChat` override. `PlayerChat` subscribes to `Players.Said`, and
  `HandleSay` loses its menu-input and command steps and sets `Blocked` for text mutes and admin tags.
- anticheat and stronghold subscribe to `Runtime.Map.Started`, either in `Load` or in the member that
  does the resetting.

### 2.5 Publishing returns a Subscription

- `App/ServiceExchange.hpp`: both `Publish` overloads return a `[[nodiscard]] Subscription` that
  withdraws the entry. Both `Unpublish` overloads are deleted.
- These keep the returned `Subscription` as their last member: admin-system's `AdminActionsService`,
  `PermissionService`, `AdminMenuSection` and `ReportMenuSection`, and stronghold's `Shop`. Their
  `Unpublish()` methods and the `~App` calls go.
- The publish points stay where they are; admin actions still publish only once the database is ready.
- `tests/App/ServiceExchangeTests.cpp`: dropping the subscription withdraws the entry.

### 2.6 The runtime builds the Panorama menu

- `Subscription Runtime::UsePanorama(PanoramaMenuLayout& layout, uint64_t addonId)` builds a
  `PanoramaMenu` from the runtime's own services, makes `Menus` prefer it, and returns a
  `Subscription` that owns both. It is a `Subscription` rather than a runtime member because the menu
  refers to the plugin's layout, which dies before the runtime.
- Delete `Runtime::PanoramaMenuServices()`.
- admin-system and main-menu replace `std::optional<PanoramaMenu>` and `PreferPanorama` with one
  member, declared after the layout so it drops first:

  ```cpp
  VoltMod::Subscription Panorama;
  ...
  Panorama = Runtime.UsePanorama(Layout, menu.addonId);
  ```

- main-menu's `Panorama->IsOpen(slot)` becomes `Runtime.Menus.IsOpen(slot)`, and
  `Hub.OpenSection(..., *Panorama)` takes `Runtime.Menus`.

### 2.7 Coordinators take `App&`

- **Rule:** a plugin class that needs more than five of the App's members besides `Runtime` takes
  `App&`. Other classes keep explicit references, and pure logic keeps taking plain values.
- **stronghold:** `Panel`, `Store`, `Mover`, `Placer` and `Health` take `App&`. A new `src/Types.hpp`
  declares `App`, with the same comment as admin-system's `Core/Types.hpp`.

  ```cpp
  // before
  StructurePanel Panel{Runtime, Config, Keys, Structures, Money, Placer, Sabotage, Mover,
                       RocketLaunchers, Seats, Loadouts, Store, Screen};
  // after
  StructurePanel Panel{*this};                  // uses _app.Keys, _app.Structures, ...
  JumpPadSystem JumpPads{Runtime, Structures};  // a leaf keeps its short, explicit list
  ```

- **Construction:** a constructor that takes `App&` stores it and subscribes. It calls no other
  member, because members declared below it are not built yet.
- **Not everywhere:** each `.cpp` that uses `App&` includes `App.hpp`, and so every system header.
  An edit to any of them then recompiles all of those files, and the constructors stop showing what
  each class depends on. The trade is worth it for five coordinators, not for 31 systems.

### Docs and rules

- `voltmod/docs/plugin.md`: entry point, the table of overrides, `LoadConfig` and load steps.
- `voltmod/docs/`: `config.md`, `host.md` (cross-plugin services), `chat.md`, `commands.md`,
  `sdk/messaging.md`, `architecture.md` (the load sequence and Lifetimes) and `menu.md` (`UsePanorama`).
- `voltmod/README.md` and `CHANGELOG.md`.
- `voltmod/templates/plugin/src/App.hpp` and `App.cpp`.
- `voltmod/.claude/rules/design.md`: "Runtime and injection".
- cs2-plugins:
  - `.claude/rules/framework-patterns.md`: the Lifecycle, Subscriptions, Configuration, and "Menus
    and effects" sections;
  - `.claude/rules/cpp.md`: Dependencies, for the `App&` rule;
  - `docs/getting-started-plugin.md`.

### Done when

- All five plugins load, and `volt reload` works cleanly for each.
- bhop builds with no `App.cpp`.
- A broken `settings.jsonc` refuses the plugin with `Configuration: <path>: <reason>`, and `Load`
  never runs.
- A feature that did not bind still shows in the `load` status section with its reason.
- Menu text input, `!` commands, admin text mutes and admin chat tags still work.
- A map change resets stronghold and anticheat tracking.
- `volt unload admin-system` logs no leftover service.
- The admin and main menus open on Panorama, their tabs and the report button work, and a player
  without the addon still gets center HTML.
- stronghold's shop, placing, moving, structure panel and structure damage work as before.

## Phase 3 - Permissions and admin-only code

### 3.1 Permission checks in every plugin

- Move `contracts/include/Contracts/IPermissions.hpp` into the framework as
  `voltmod/include/VoltMod/Players/Permissions.hpp`. It has `InterfaceName = "voltmod.IPermissions/1"`
  and `bool HasPermission(int64_t steamId, std::string_view permission)`. Delete the contracts header.
- `Runtime` installs a default `Policy.HasPermission` in its constructor. On each call it asks
  `Exchange.Get<IPermissions>()`; when nothing is published it denies, and logs that once.
- Delete `CommandManager::CommandsMissingPolicy` and the `Permissions` load step.
- admin-system's `PermissionService` implements `VoltMod::IPermissions`. Its own `InstallPolicy` still
  points `HasPermission` at `Access`.

### 3.2 Admin-only types move to admin-system

- The types move into `plugins/admin-system/src/Admin/`, and their definitions replace the aliases in
  `Actions/ActionContext.hpp`:
  - `Action`, `ParamAction`, `ActionContext` and `ActionDispatcher` go to `Admin/Actions/`.
  - `EffectScope`, `EffectInstance`, `EffectManager`, `EffectChoice`, `EffectDescriptor` and
    `EffectDispatcher` go to `Admin/Effects/`.
  - `ActionRows` goes to `Admin/Menu/`.
- The broadcast callback moves from `Policy::Broadcast` to `ActionDispatcher`, and `InstallPolicy`
  sets it there.
- `voltmod/tests/Players/EffectManagerTests.cpp` moves to `plugins/admin-system/tests/`, with
  `src/Admin/Effects/EffectManager.cpp` added to `admin-system-tests`. It depends only on the scheduler.
- In the framework:
  - Delete the moved headers and sources.
  - `Api.hpp` stops including `Players/EffectDispatcher.hpp`, and `Menu/Api.hpp` stops including
    `ActionRows.hpp`.
  - Update the API surface tests to match.
- `Flow<TState>` and `MenuPresets` stay. They are general menu helpers with their own framework tests
  (`tests/Menu/FlowTests.cpp`).

### Docs and rules

- `voltmod/docs/`: `players.md`, `commands.md`, `menu.md` and `architecture.md`.
- `voltmod/.claude/rules/design.md`: "Registration and authorization".
- `.claude/rules/framework-patterns.md`: "Authorization" and "Menus and effects".
- `contracts/README.md`.

### Done when

- A permission-gated command in a plugin other than admin-system works while admin-system is loaded.
  Without admin-system it is denied, with one log line.
- Admin menu actions and effects still broadcast.
- `<VoltMod/Api.hpp>` compiles with no admin type in it.

## Phase 4 - Host

### 4.1 Refused plugins in `volt list`

`PluginLoader` keeps the last refusals, both those from `Resolve` and those from a failed `LoadOne`.
`PrintLoaded` prints them, with their reasons, after the loaded list. A later successful `volt load`
clears the entry.

### 4.2 Refuse a player at connect

- Host:
  - Hook `IServerGameClients::ClientConnect` (before the call) in `EngineHooks`.
  - Add `IHostEvents::OnClientConnecting`, which offers `slot, steamId, name` to each plugin in load
    order. The first refusal makes the hook return `false` and writes its reason into the engine's
    reject buffer.
  - Bump `HostAbiVersion`.
- SDK: `PlayerManager::Connecting` is an `Event<ConnectRequest&>`, raised before the player is in the
  roster:

  ```cpp
  /** A player asking to join. Set Rejected to refuse them; they see Reason. */
  struct ConnectRequest
  {
      int Slot = -1;
      int64_t SteamId = 0;
      std::string_view Name;
      bool Rejected = false;
      std::string Reason;
  };
  ```

- admin-system: the ban check moves from `OnPlayerConnect` to `Players.Connecting`. `KickDeferred`
  stays for bans issued while the target is online.

### 4.3 More `plugin.json` fields

| Key | Type | Default | Meaning |
| --- | --- | --- | --- |
| `$schema` | string | none | Points editors at the plugin schema. The host ignores it. |
| `website` | string | `""` | The plugin's page or repository. Credit, like `author`. |
| `license` | string | `""` | An SPDX id such as `MIT`, or `proprietary`. Credit only. |
| `logLevel` | string | `"info"` | The level the plugin starts at: `info`, `warn` or `error`. `volt log` changes it until the next load. |

- Host: `PluginDocument` gains the four keys, with `$schema` accepted and ignored; an unknown level is
  a manifest error that names the key, like any other. The host starts each plugin at its `logLevel`.
- New `voltmod/templates/plugin.schema.json`, a plain JSON Schema written like
  `gamedata/gamedata.schema.json` (`additionalProperties: false`). It sits at the top of `templates/`,
  so the scaffold does not copy it into each plugin. `$schema` points at it by URL:
  `https://raw.githubusercontent.com/voltygg/voltmod/main/templates/plugin.schema.json`.
- `templates/plugin/plugin.json` and all five plugins' `plugin.json` get `$schema`, `website` and
  `license`; stronghold's empty `description` and `author` are filled in.
- `dependencies` and `optionalDependencies` stay as they are.

### Tests and docs

- `tests/Host/Loading/InstalledPluginsTests.cpp`: the new keys are read, `$schema` is accepted, and an
  unknown level is refused, naming the key.
- Docs:
  - `voltmod/docs/plugin.md`: the `plugin.json` table.
  - `voltmod/docs/host.md`: the list output.
  - `voltmod/docs/players.md`: `Connecting`.

### Done when

- A plugin with a broken manifest shows in `volt list` with its reason.
- VS Code completes `plugin.json` keys from the schema.
- A plugin with `"logLevel": "warn"` prints no info lines until `volt log <name> info`.
- A banned account is refused at connect, sees the ban reason, and never appears in the roster.
- Bots still join, also with the legacy dummy loaded (`tools/legacy-dummy`).

## Phase 5 - Player language from the client

- main-menu, on `Players.FullyConnected`, when the player has no language yet and
  `Hooks.ClientConVars.Available()`:
  - query `cl_language`;
  - map Steam's name to a language that has translation files (`english` to `en`, `russian` to `ru`;
    extend the table when a language is added);
  - call `Translations.SetPlayerLanguage`.
- The language table is shared by every plugin, so one query covers all of them.
- Done when:
  - a client with Steam set to Russian gets Russian messages from every plugin without opening
    settings;
  - picking a language in settings still overrides it.

## Phase 6 - Generated game events

### Generator

`voltmod framework eventgen` (`cli/voltmod/framework/eventgen.py`) sits beside `schemagen` and, like
it, runs by hand after a game update. It is one short script:

- **Read:** every `resource/*.gameevents` in `game/core/pak01_dir.vpk` and `game/csgo/pak01_dir.vpk`
  under `CS2_SERVER_PATH`, through the `vpk` package (the CLI's one new dependency). When two files
  define the same event, the later one wins, in the order core, game, mod.
- **Parse:** KeyValues with a small tokenizer: quoted strings and braces, with `//` comments dropped.
  The `local` flag is not a field.
- **Write:** `include/VoltMod/Events/EventTypes.hpp` and `src/Events/EventTypes.cpp` from two Jinja
  templates, as `schemagen` does, replacing the hand-written pair. Every event keeps today's shape:
  `Name`, fields, and `From(IGameEvent&)`.
- **Names:** generated only.
  - The struct name comes from the event name: `player_death` becomes `PlayerDeath`.
  - A field name comes from its key, split on `_` and capitalized: `dmg_health` becomes `DmgHealth`,
    and `oldteam` becomes `Oldteam`.
  - A player key becomes a slot: `userid` becomes `Slot`, and any other becomes `<Key>Slot`, so
    `attacker` becomes `AttackerSlot`.
- **Types:**

  | Game type | C++ |
  | --- | --- |
  | `string` | `std::string` |
  | `bool` | `bool` |
  | `byte`, `short`, `long`, `int` | `int` |
  | `float` | `float` |
  | `uint64` | `uint64_t` |
  | `player_controller`, `player_controller_and_pawn` | an `int` slot, through `GetPlayerSlot` |

  `player_pawn` and `ehandle` fields (14 in all) are written as `// skipped:` comments, as
  `schemagen` does, until a plugin needs one.
- **Two small tables in the generator**, not a manifest file:
  - typed keys: `team` and `oldteam` hold a `VoltMod::Team`, and `hitgroup` a `VoltMod::HitGroup`;
  - hand-written events: `bullet_impact`, because the engine truncates its `userid` to a byte. It
    moves to `Events/BulletImpact.hpp`, which the generated header includes, and keeps
    `TruncatedUserId` next to its best-effort slot, renamed `ShooterSlot`.
- **Tests,** in `cli/tests/`: a KeyValues sample, the naming, a typed key, a skipped type, and a
  hand-written event left out.

### Valid subjects

- `GameEvents::On<T>` returns early when the event's `Slot` is not a valid slot, so a handler never
  sees -1 for the player the event is about.
- Optional parties, such as `AttackerSlot`, keep -1.
- `BulletImpact` has no `Slot` (its best-effort slot is `ShooterSlot`), so it is not filtered.

### Plugins

The event fields plugins read today, and their names afterwards:

| Today | Afterwards | Read at |
| --- | --- | --- |
| `PlayerDeath::VictimSlot` | `Slot` | admin-system `App.cpp:173-175`; stronghold `App.cpp:100-101` and `BountySystem.cpp:36`; anticheat `DetectionFeed.cpp:440` |
| `PlayerHurt::VictimSlot` | `Slot` | anticheat `DetectionFeed.cpp:401` |
| `PlayerHurt::Hitbox` | `Hitgroup` | anticheat `DetectionFeed.cpp:408` |
| `PlayerTeam::OldTeam` | `Oldteam` | stronghold `App.cpp:122` |
| `BulletImpact::Slot` | `ShooterSlot` | anticheat `DetectionFeed.cpp:378` |

- `AttackerSlot`, `Weapon`, `Headshot`, `Penetrated`, `Team`, `Disconnect`, `TruncatedUserId` and
  `X`/`Y`/`Z` keep their names. `PlayerHurt::DamageHealth` becomes `DmgHealth` and `VoteCast::Option`
  becomes `VoteOption`; nothing reads either today.
- anticheat's own `Shot::VictimSlot` is not an event field and stays.
- Remove the `IsValidSlot` guards that only protect an event's `Slot`, starting with admin-system's
  `e.VictimSlot >= 0` check.

### Docs and rules

- `voltmod/docs/sdk/events.md`: every event is generated; regenerate after a game update, as with the
  schema; `bullet_impact` is written by hand.
- `voltmod/.claude/rules/design.md`: "Players and events".
- `.claude/rules/framework-patterns.md`: "Subscriptions and hooks".
- `voltmod/.claude/skills/gamedata-update/SKILL.md`: run `eventgen` next to `schemagen`.

### Done when

- The generated header and source build on both platforms.
- Every event-driven feature still works:
  - stronghold kill rewards and bounties;
  - bhop's hop boost;
  - anticheat's bullet impacts;
  - admin-system's effect cancels on death and round end.

## Phase 7 - Cleanup

- admin-system's `~App` replaces `Runtime.Players.Clear()` with a loop over `Players.All()` into its
  own `OnPlayerDisconnect`.
- `ConVars::ExecuteClientCommand(slot, cmd)` becomes `Controller::ExecuteCommand(cmd)`: a verb lives
  on the wrapper it acts on. The one caller is main-menu's `HubMenu.cpp:139`.
- Delete `Logger<T>` (`App/Logger.hpp`, `tests/App/LoggerTests.cpp`, the Logging part of
  `docs/plugin.md`) and `EntitySystem::AlivePawns()` (and its line in `docs/sdk/entities.md`).
- Doc fixes:
  - `voltmod/docs/host.md` shows `InterfaceName` as `std::string_view`.
  - `.claude/rules/framework-patterns.md` names `OpenSession`/`Open`.

## Commits and the final test

- One branch, `feat/plugin-api-redesign`, in voltmod, in cs2-plugins, and in any plugin submodule a
  phase touches.
- Commit in groups, one per phase or topic, voltmod first, each group building and passing
  `uv run poe test` on Windows.
- No release and no relock: cs2-plugins builds against the editable checkout the whole way.
- Once all seven phases are complete, and not before, a single `/deploy-test` to a test server,
  followed by the live checks from each phase's "Done when" through `/rcon-debug`. Phases are checked
  on the local Windows server as they land.

## On demand

Each of these waits for its first user in this repo. The design is noted so it lands the same way.

- **Dependency order and versions.** Load providers before dependents (Kahn's algorithm,
  alphabetical ties, a cycle refused), and version requirements such as
  `"dependencies": { "admin-system": ">=1.2.0" }`. For the first plugin with a required dependency.
- **Entity events.** `EntitySystem` gets `Event<Entity> Created`, `Spawned` and `Deleted`, sharing one
  `IEntityListener` added on the first subscription and removed after the last.
- **Client command listener.** `CommandManager` gets `Event<ClientCommand&> Received` (`Sender`,
  `Name`, `Arguments`, `Blocked`) for every command except chat and votes, such as `jointeam`.
- **Blocking game events.** `GameEvents.Before<T>` over a hook on `IGameEventManager2::FireEvent`,
  with `Blocked` and `DontBroadcast`, such as to hide kill-feed entries.
- **User messages.** Block a message type or change its recipients, without per-field protobuf access.
- **Transmit crash guards.** Plugify sends a newly spawned pawn for one tick and always sends dead
  pawns; VoltMod's `Visibility` does neither. If hiding a pawn ever crashes a client, look here first.

## Risks

- **Settings loaded in a member initializer:** a member declared above `Config` cannot read it. When
  the settings fail to load, later members are still built with default settings before the plugin
  is refused, so constructors must not act outside the plugin. Commands and subscriptions are fine,
  because the refusal removes them.
- **Setup in constructors:** a service that logs or fails while it is built needs the log handler
  before it and the load steps after it. The module installs the handler first and reads the steps
  once construction ends.
- **`App&` coordinators:** a constructor that calls a member declared below it reads an object that
  is not built yet. Check each of the five for that when converting it.
- **The `ClientConnect` hook:** it sits under the SourceHook hooks of legacy Metamod plugins, because
  VoltMod starts first. Test it with the legacy dummy and on Linux.
- **The event generator:** it depends on the `.gameevents` format and the VPK layout. Commit the
  generated files and regenerate only on purpose, as with the schema.
- **The slot filter:** an event raised while a player leaves may no longer decode to a slot and is
  then dropped. Check `player_team` with `Disconnect` set, which stronghold reads.
- **Breaking phases:** each phase breaks the API. Every phase updates all five plugins in the same
  change and builds against the editable checkout; nothing is released.

## Out of scope

- Attribute or reflection registration, string-keyed schema or event access, DI containers,
  sync/async twins of each call, and plugin-created convars.
- A per-plugin context struct such as `Game{Runtime, Config, Screen, Money, Structures}`: it would be
  a second bag next to the App, which the plugin rules already rule out.
- Small conveniences that save a few characters at one or two call sites: a `Caller::Done()`,
  `operator*` on arguments, iteration on `Targets`, and `SendKey`/`BroadcastKey` overloads without
  the tokens argument.
- Grouping `BroadcastKey` recipients by language: one `SendKey` per player is simpler, and a
  broadcast is rare.
- Making framework-only members private with friend declarations: the headers already say who calls
  them, and the one misuse, admin-system's `Players.Clear()`, is fixed in phase 7.
- Per-plugin gamedata or schema files, and generating every schema class. Framework edits cover the
  plugins in this repo, and the manifest keeps a game update from refusing plugins over fields they
  don't use.
- A SAT solver, `conflicts`, or a package manager for plugins.
- Chrono durations in `Scheduler` (fact 17).
- A hand-written export manifest for other languages. If bindings come, generate them from the C++
  headers.

## Decisions

Made on 2026-09-26:

- The simplest version of each change; anything without a user today is in "On demand".
- No release: a single `/deploy-test` once all phases are complete, on a new branch with grouped
  commits.
- Translated messages are `SendKey`/`BroadcastKey`, one signature each; `Send`/`Broadcast` keep
  sending finished text.
- `Logger<T>` and `EntitySystem::AlivePawns()` are deleted.
- `plugin.json` gains `$schema`, `website`, `license` and `logLevel`.
- The runtime's services are ready when built, `UsePanorama` replaces hand-assembled Panorama menus,
  and plugin classes that need more than five App members take `App&` (phase 2).
- Game event structs are generated from the game's own files, with generated names only; the player
  an event is about is `Slot`, and `bullet_impact` stays hand-written (phase 6).

Still open: sending admin-system's `ChatService` broadcasts in each player's language through
`BroadcastKey`. It is planned for later, because those lines are built from several parts.
