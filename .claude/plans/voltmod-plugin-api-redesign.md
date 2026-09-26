# VoltMod plugin API redesign

Status: planned 2026-09-26 from an API review against SwiftlyS2, CounterStrikeSharp and Plugify
(`references/`). Not started. Facts checked against voltmod `cf7cdac` and cs2-plugins `1e86236`.

## Goal

Plugin code gets shorter and harder to get wrong, without new subsystems. When this plan is done:

- A plugin declares no entry macro and no constructor, and writes a `Load()` only when it needs one.
- A command handler has no trailing return type and no `return Reply::Silent();`.
- A translated message is one call, and a message to everyone arrives in each player's language.
- `.Permission("x")` works in every plugin as soon as admin-system is loaded.
- Map start and chat are events like the other lifecycle signals. No plugin redoes menu input or `!`
  command dispatch.
- Publishing a cross-plugin service returns a `Subscription`, so nothing is withdrawn by hand.
- Admin-only types live in admin-system, and `<VoltMod/Api.hpp>` shows only general tools.
- Every game event has a generated struct, and no event reaches a handler with an invalid slot for
  the player it is about.
- A banned player is refused at connect and sees the reason.
- `plugin.json` carries a website, a license, a starting log level and minimum versions for its
  dependencies, and editors complete it from a schema.

Rules for this change:

- Breaking changes are fine; the framework has no external consumers. Every caller is updated in the
  same change, with no aliases or shims.
- Change existing types and add no subsystem. The one new tool, the event generator, follows `schemagen`.
- Keep what the review found better than the references: typed command arguments, frame-local
  wrappers with storable refs, generated setters that replicate, a `Subscription` for every
  registration, `PerSlot`, `Options` and `ServiceExchange`.
- One way to block: an event struct carries a `Blocked` flag, as `DamageHit` does. Only raw `Unsafe`
  hooks return `HookResult`.
- The rules files (`voltmod/.claude/rules/design.md`, `.claude/rules/framework-patterns.md`) change in
  the same phase as the API they describe, because future sessions load them.
- Work on a new branch, `feat/plugin-api-redesign`, in every repo the change touches. Build against
  the editable `voltmod` checkout, and commit in groups, one per phase or topic, voltmod first.
- No release and no relock. A single `/deploy-test`, run once all phases are complete, not per phase.

## Facts this plan relies on

1. **Commands.** All 28 command handlers in `plugins/` declare `-> Result<Reply>`. `Caller::Ok`
   returns `Result<Reply>` but `Caller::Fail` returns `std::unexpected<Error>`
   (`voltmod/src/Commands/CommandBuilder.cpp:13-23`), so a lambda returning both cannot deduce its
   type. `Fail` is returned only from handlers and from admin-system's `Punish` helper, which already
   returns `Result<Reply>`.
   - Nine handlers end with `return Reply::Silent();`. `FreezeCommands.cpp:130` and
     `ReportCommand.cpp:40` mix it with `c.Ok`/`c.Fail`.
   - `c.Tr.Get` appears twice, at `PunishmentCommands.cpp:91` and `:114`.
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
5. **Two-phase start.** The module constructs the App and then calls `Load`
   (`voltmod/src/App/Plugin.cpp`, `AttachImpl`), and settings load inside `Load`.
   - Members therefore subscribe before config exists, and `BhopManager`, `Detectors` and
     `CheatSimulator` each grew an `Initialize()`.
   - Two reload paths repeat the settings path: `BhopManager.cpp:156` and anticheat
     `Engine/Commands.cpp:29`.
6. **Entry macro.** `VOLTMOD_PLUGIN` (`voltmod/include/VoltMod/App/PluginEntry.hpp`) differs between
   plugins only by the class name.
   - All five plugins declare `<PascalCase name>::App` in `src/App.hpp`, and the scaffold uses the
     same names (`voltmod/cli/voltmod/scaffold/scaffold.py`, `template_values`).
   - No test target compiles `App.cpp`.
   - Precedent: `voltmod/cmake/DoctestMain.cpp` is a framework-owned source compiled into every test
     target.
7. **Exchange.** `Publish` and `Unpublish` are a manual pair. admin-system's `~App` calls four
   `Unpublish`es (`App.cpp:29-38`) and stronghold's calls one. The host logs anything left published
   as a bug.
8. **Admin-only types.** `Action`, `ParamAction`, `ActionContext`, `ActionDispatcher`,
   `EffectDescriptor`, `EffectDispatcher`, `EffectManager` and `ActionRows` are used only by
   admin-system, which already aliases them into its own namespace
   (`plugins/admin-system/src/Admin/Actions/ActionContext.hpp`).
   - `<VoltMod/Api.hpp>` includes `Players/EffectDispatcher.hpp`, and `<VoltMod/Menu/Api.hpp>`
     includes `ActionRows.hpp`.
   - `Policy::Broadcast` is read only by `ActionDispatcher`.
   - `voltmod/tests/Players/EffectManagerTests.cpp` has 8 cases.
9. **Events.** There are 14 hand-written structs in `Events/EventTypes.hpp`, and plugins use 10 of
   them. `EventTypes.cpp` decodes `userid` with `GetPlayerSlot`, which gives -1 for a missing player.
   Plugins carry 50 `IsValidSlot` guards, most of them protecting `PerSlot` indexing.
   - SwiftlyS2 generates 276 event classes from the game's `.gameevents` KeyValues files, and its
     generated comments preserve the field types. There are twelve:
     `player_controller`, `player_controller_and_pawn`, `player_pawn`, `short`, `long`, `int`,
     `byte`, `bool`, `float`, `string`, `uint64` and `ehandle`.
10. **Host loading.** Plugins load alphabetically (`voltmod/src/Host/Loading/PluginDependencies.hpp`).
    - `LoadList.Refused` is logged once and then dropped (`PluginLoader.cpp:108-113`).
    - `volt list` prints only loaded plugins (`VoltCommand.cpp`, `PrintLoaded`).
    - No plugin has a required dependency today; anticheat lists admin-system as optional.
11. **Connect.** The host hooks `IServerGameClients::OnClientConnected` (`EngineHooks.cpp:126`), which
    runs after the player is already in. admin-system therefore kicks a banned player on the next tick
    (`App.cpp`, `OnPlayerConnect`; `PunishmentManager.cpp:260`). `ClientConnect(CPlayerSlot, const char*
    name, uint64 xuid, const char* networkId, bool, CBufferString* rejectReason)` returns a bool and
    carries a reject reason (`references/hl2sdk/public/eiface.h:569`).
12. **Language.** A player's language changes only when they pick one in main-menu (`HubMenu.cpp:79`)
    or in stronghold's shop (`Shop.cpp:270`), although every plugin ships `en.json` and `ru.json`.
    `Hooks.ClientConVars.Query` can already ask a client for a convar.
13. **Entities and client commands.**
    - `IEntityListener` (created, spawned, deleted, parent changed) registers through
      `CGameEntitySystem::AddListenerEntity` and needs no gamedata
      (`references/hl2sdk/public/entity2/entitysystem.h:196,379`).
    - The host offers every dispatched console command to each plugin (`EngineHooks.cpp`,
      `RunCommand`), but the plugin module looks only at `vote`, `callvote`, `say` and `say_team`.
14. **Plumbing.** Headers mark public members as "framework only" or `@internal`: `CommandManager`,
    `PlayerManager`, `Map::SetCurrent`, `Precache::AddTo`, `EntitySystem`, `ClientConVars` and
    `SlotEvents::Raise`.
    - admin-system's `~App` calls `Runtime.Players.Clear()`, and it is admin-system's only
      `Disconnected` subscriber.
    - `PlayerManagerTests.cpp` and `PolicyAuthorizeTests.cpp` drive the roster through `Add`/`Remove`.
15. **No chrono literals.** Plugin code bans `using namespace`, so `Scheduler.Repeat(1min, ...)` would
    have to be `Repeat(std::chrono::minutes{1}, ...)`, which reads no better than `60'000`. That
    review item is dropped.
16. **Manifest.** `plugin.json` has `name`, `version`, `logTag`, `description`, `author`,
    `dependencies`, `optionalDependencies` and `database`.
    - The host reads it strictly through Glaze (`src/Host/Loading/InstalledPlugins.cpp`,
      `PluginDocument`), so an unknown key refuses the plugin.
    - `tests/Host/Loading/InstalledPluginsTests.cpp:62` installs the array form of `dependencies`.
    - The CLI has no JSON-schema library; `gamedata/gamedata.schema.json` serves editors only.
    - stronghold's `description` and `author` are empty.

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

_rt.Messages.SendKey(slot, enabled ? "bhop.granted" : "bhop.revoked", VoltMod::MessageKind::Center);
```

## Phase 1 - Commands and messages

### Framework

1. `Commands/CommandBuilder.hpp`, `src/Commands/CommandBuilder.cpp`:
   - `Caller::Fail` returns `Result<Reply>`.
   - Add `Result<Reply> Caller::Done() const`, a success with no reply, for handlers that also return
     `Ok` or `Fail`.
   - Delete `Reply::Silent()`.
   - Rename `Caller::Tr` to `Caller::Translations`; plugins call `c.Text(key)` instead.
   - `CommandHandlerArgs` also exposes the handler's return type. `Bind` accepts handlers that return
     nothing (a silent success), `Reply` or `Result<Reply>`, and the primary template's
     `static_assert` says so.
2. `Commands/Args.hpp`:
   - Single-value arguments (`Target`, `Duration`, `SteamId`, `Int`, `U64`, `Word`, `Rest`) get
     `operator*`, and `Target` also gets `operator->`, so a handler writes `t->Name()`.
   - `Targets` gets `begin()` and `end()`.
   - `Opt<T>` gets `explicit operator bool`, `operator->` and `ValueOr(fallback)`, which returns the
     inner value: `why.ValueOr("No reason")` is a `std::string`.
   - `.Value` stays public.
3. `Messaging/Messages.hpp`, `src/Messaging/Messages.cpp`. `Send` and `Broadcast` keep sending
   finished text; the `Key` forms translate:

   | Call | Sends |
   | --- | --- |
   | `Send(slot, text, kind = Chat)` | `text` as written (unchanged) |
   | `SendKey(slot, key, tokens = {}, kind = Chat)`, `SendKey(slot, key, kind)` | `key` in the slot's language |
   | `Broadcast(text, kind = Chat)` | `text` as written, to everyone (unchanged) |
   | `BroadcastKey(key, tokens = {}, kind = Chat)`, `BroadcastKey(key, kind)` | `key` in each player's language |

   - `BroadcastKey` groups the 64 slots by `Translations::PlayerLanguage` (empty means the server
     language), translates once per group, and posts one message per group. The engine drops empty
     slots, which `Broadcast` already relies on. Center HTML goes out per connected slot, as
     `Broadcast` does it.
   - Delete `Reply` and `ReplyKey`. `CommandManager.cpp:97` calls `Send`.
4. Tests, in `tests/Commands/CommandBuilderTests.cpp`:
   - A handler returning nothing registers and replies nothing.
   - A lambda returning both `c.Ok` and `c.Fail` compiles without a trailing return type.
   - `Opt<Rest>::ValueOr`.
   - `Target`'s `operator->`.

### Plugins

- Every handler loses `-> Result<Reply>`.
- `Reply::Silent()` becomes either a handler that returns nothing or `return c.Done();`; the second
  applies in `FreezeCommands.cpp` and `ReportCommand.cpp`.
- `t.Value->` becomes `t->`, `why.Value ? why.Value->Value : x` becomes `why.ValueOr(x)`, and
  `c.Tr.Get(k)` becomes `c.Text(k)`.
- Messages:
  - `ReplyKey` becomes `SendKey` in `AdminActionsService.cpp`, `ReportFlow.cpp` and
    `ReportMenuSection.cpp`.
  - `Reply` becomes `Send` in `FreezeManager.cpp` and `ChatService.cpp`.
  - `Broadcast` in `ChatService.cpp` and `PlayerChat.cpp` stays as it is.
  - bhop's `Send(slot, Translations.Get(key, slot), Center)` becomes `SendKey(slot, key, Center)`.
  - stronghold's `Hud::NotifyAll` calls `BroadcastKey` once instead of `ReplyKey` per player.
- admin-system's `ChatService` broadcasts stay one server-language line. They build composite lines,
  so sending them in each player's language is a separate change.
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

## Phase 2 - Plugin shape

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
- `CLASS`/`HEADER` arguments that override the convention wait until a plugin needs them.

### 2.2 Build the App in one step

- `Plugin::Load` becomes `virtual bool Load() { return true; }`.
- `LoadConfig` returns the config:
  `template <class TConfig> TConfig LoadConfig(Runtime&, TConfig config = {}, const LoadConfigOptions& = {})`.
  It still records the required `Configuration` step and loads translations; a failure leaves the
  defaults in place.
- `PluginModule::AttachImpl`: when a required step has already failed by the time the App is built,
  the plugin is refused with that reason before host events are subscribed and before `Load` runs.
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

### 2.3 Map start and chat are events

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

### 2.4 Publishing returns a Subscription

- `App/ServiceExchange.hpp`: both `Publish` overloads return a `[[nodiscard]] Subscription` that
  withdraws the entry. Both `Unpublish` overloads are deleted.
- These keep the returned `Subscription` as their last member: admin-system's `AdminActionsService`,
  `PermissionService`, `AdminMenuSection` and `ReportMenuSection`, and stronghold's `Shop`. Their
  `Unpublish()` methods and the `~App` calls go.
- The publish points stay where they are; admin actions still publish only once the database is ready.
- `tests/App/ServiceExchangeTests.cpp`: dropping the subscription withdraws the entry.

### Docs and rules

- `voltmod/docs/plugin.md`: entry point, the table of overrides, `LoadConfig` and load steps.
- `voltmod/docs/`: `config.md`, `host.md` (cross-plugin services), `chat.md`, `commands.md` and
  `sdk/messaging.md`.
- `voltmod/README.md` and `CHANGELOG.md`.
- `voltmod/templates/plugin/src/App.hpp` and `App.cpp`.
- `voltmod/.claude/rules/design.md`.
- cs2-plugins: the Lifecycle, Subscriptions and Configuration sections of
  `.claude/rules/framework-patterns.md`, and `docs/getting-started-plugin.md`.

### Done when

- All five plugins load, and `volt reload` works cleanly for each.
- bhop builds with no `App.cpp`.
- A broken `settings.jsonc` refuses the plugin with `Configuration: <path>: <reason>`, and `Load`
  never runs.
- Menu text input, `!` commands, admin text mutes and admin chat tags still work.
- A map change resets stronghold and anticheat tracking.
- `volt unload admin-system` logs no leftover service.

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
  struct ConnectRequest
  {
      int Slot = -1;
      int64_t SteamId = 0;
      std::string_view Name;
      bool Rejected = false;
      std::string Reason;

      void Reject(std::string reason)
      {
          Rejected = true;
          Reason = std::move(reason);
      }
  };
  ```
- admin-system: the ban check moves from `OnPlayerConnect` to `Players.Connecting`. `KickDeferred`
  stays for bans issued while the target is online.

### 4.3 Load in dependency order

- `PluginDependencies::Resolve` orders `Allowed` so every plugin comes after its `dependencies`. It
  uses Kahn's algorithm and breaks ties alphabetically among plugins that are ready.
- A cycle of required dependencies refuses the plugins in it and names the cycle.
- `optionalDependencies` do not change the order.
- `volt reload` brings a plugin and its dependents back in that same order.
- Nothing changes for today's plugins, since none has a required dependency.

### 4.4 More `plugin.json` fields

| Key | Type | Default | Meaning |
| --- | --- | --- | --- |
| `$schema` | string | none | The plugin schema, for editor completion and checks. The host ignores it. |
| `website` | string | `""` | The plugin's page or repository. Credit, like `author`; the host does not print it. |
| `license` | string | `""` | An SPDX id such as `MIT`, or `proprietary`. Credit only. |
| `logLevel` | string | `"info"` | The level the plugin starts at: `info`, `warn` or `error`. `volt log` changes it until the next load. |
| `dependencies` | object | `{}` | Plugin name to version requirement: `"*"` or `">=1.2.0"`. A missing plugin or a lower version refuses this one. |
| `optionalDependencies` | object | `{}` | The same shape. A lower version logs a warning, and the plugin still loads. |

- The dependency lists become objects, as in npm's `package.json`:

  ```json
  "dependencies": { "admin-system": ">=1.2.0" },
  "optionalDependencies": {}
  ```

- A requirement is either `*` or `>=MAJOR.MINOR.PATCH`. A `>=` requirement on a provider whose
  `version` is not `MAJOR.MINOR.PATCH` refuses the dependent and says why. The CMake configure check
  also requires `version` to be `MAJOR.MINOR.PATCH`.
- Host (`src/Host/Loading/InstalledPlugins.*`): `PluginDocument` gains `website`, `license`,
  `logLevel`, the two maps, and the key `$schema`, which it accepts and ignores. A requirement in
  neither form, or an unknown level, is a manifest error that names the key, like any other.
- The host starts each plugin at its `logLevel`. `volt log` changes it until the next load or reload,
  which reads the manifest again.
- `PluginDependencies::Resolve` compares versions. A refusal reads `Refusing 'anticheat': it
  requires 'admin-system' >=1.2.0, and 1.1.0 is installed.` An optional dependency below its version
  logs the same sentence as a warning.
- New `voltmod/templates/plugin.schema.json`, written like `gamedata/gamedata.schema.json` with
  `additionalProperties: false`. It sits at the top of `templates/`, so the scaffold does not copy it
  into each plugin. `$schema` points at it by URL:
  `https://raw.githubusercontent.com/voltygg/voltmod/main/templates/plugin.schema.json`.
- `templates/plugin/plugin.json` gets `$schema`, `website`, `license` and the two empty objects.
- cs2-plugins: every `plugin.json` gets `$schema`, `website` and `license`. anticheat's
  `optionalDependencies` becomes `{ "admin-system": "*" }`, and stronghold's empty `description` and
  `author` are filled in.

### Tests and docs

- `tests/Host/Loading/PluginDependenciesTests.cpp`:
  - A provider loads before a dependent whose name sorts first.
  - Ties stay alphabetical.
  - A cycle is refused.
  - A required dependency below its version is refused with both versions named.
  - An optional dependency below its version still loads.
- `tests/Host/Loading/InstalledPluginsTests.cpp`:
  - The object form and `logLevel` are read, and `$schema` is accepted.
  - A malformed requirement, an unknown level and the old array form are refused, naming the key.
- Docs:
  - `voltmod/docs/plugin.md`: the `plugin.json` table.
  - `voltmod/docs/host.md`: the list output, and Dependencies with versions.
  - `voltmod/docs/architecture.md`: the load sequence.
  - `voltmod/docs/players.md`: `Connecting`.
  - `docs/getting-started-plugin.md`.

### Done when

- A plugin with a broken manifest shows in `volt list` with its reason.
- A plugin requiring a newer provider is refused with both versions named.
- VS Code completes `plugin.json` keys from the schema.
- A plugin with `"logLevel": "warn"` prints no info lines until `volt log <name> info`.
- A banned account is refused at connect, sees the ban reason, and never appears in the roster.
- Bots still join.
- All of this also holds with the legacy dummy loaded (`tools/legacy-dummy`). Linux is checked in the
  final `/deploy-test`.

## Phase 5 - Player language from the client

- main-menu, on `Players.FullyConnected`, when the player has no language yet and
  `Hooks.ClientConVars.Available()`:
  - Query `cl_language`.
  - Map Steam's language name to a code (`english` becomes `en`, `russian` becomes `ru`, and so on).
  - Call `Translations.SetPlayerLanguage` when a translation file exists for that code.
- The language table is shared by every plugin, so one query covers all of them.
- Done when:
  - A client with Steam set to Russian gets Russian messages from every plugin without opening settings.
  - Picking a language in settings still overrides it.

## Phase 6 - Generated game events

### Generator

`voltmod framework eventgen` (`cli/voltmod/framework/eventgen.py`) lives beside `schemagen`:

- **Input:** the game's `.gameevents` KeyValues files, read from the VPKs under `CS2_SERVER_PATH`.
  Confirm the exact files when writing the reader.
- **Output:** `include/VoltMod/Events/EventTypes.hpp` and `src/Events/EventTypes.cpp`. They replace the
  hand-written pair and keep its shape: `Name`, typed fields, `From(IGameEvent&)`.
- **Names:** every name is generated; nothing is renamed by hand.
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
  | `player_controller`, `player_controller_and_pawn` | a slot, through `GetPlayerSlot` |
  | `player_pawn`, `ehandle` | `EntityRef` |

  An unknown type is written out as a `// skipped:` comment, as `schemagen` does.
- **Typed keys:** a short table in the generator, not a manifest file, sets three types. Names still
  come from the rules above.
  - `team` and `oldteam` hold a `VoltMod::Team`.
  - `hitgroup` holds a `VoltMod::HitGroup`.
  - `bullet_impact`'s `userid` stays a raw `int`, named `Userid`: the engine truncates it to its low
    byte, so it names no slot reliably.
- **Tests** (in `cli/tests/`): a KeyValues sample, naming, the typed keys, and a skipped type.

### Valid subjects

- `GameEvents::On<T>` drops an event whose `Slot` is not a valid slot, so a handler never sees -1 for
  the player the event is about.
- Optional parties such as `AttackerSlot` keep -1.
- `bullet_impact` has no `Slot`, since its `userid` is the raw `Userid`, so it is not filtered.

### Plugins

The event fields plugins read today, and their generated names:

| Today | Generated | Read at |
| --- | --- | --- |
| `PlayerDeath::VictimSlot` | `Slot` | admin-system `App.cpp:173-175`; stronghold `App.cpp:100-101` and `BountySystem.cpp:36`; anticheat `DetectionFeed.cpp:440` |
| `PlayerHurt::VictimSlot` | `Slot` | anticheat `DetectionFeed.cpp:401` |
| `PlayerHurt::Hitbox` | `Hitgroup` | anticheat `DetectionFeed.cpp:408` |
| `PlayerTeam::OldTeam` | `Oldteam` | stronghold `App.cpp:122` |
| `BulletImpact::TruncatedUserId` | `Userid` | anticheat `DetectionFeed.cpp:375` |

- `AttackerSlot`, `Weapon`, `Headshot`, `Penetrated`, `Team`, `Disconnect` and `X`/`Y`/`Z` keep
  their names. `PlayerHurt::DamageHealth` becomes `DmgHealth` and `VoteCast::Option` becomes
  `VoteOption`; nothing reads either today.
- anticheat's own `Shot::VictimSlot` is not an event field and stays.
- Go through the 50 `IsValidSlot` guards and remove the ones that only protect an event's subject
  slot, starting with admin-system's `e.VictimSlot >= 0` check.

### Docs and rules

- `voltmod/docs/sdk/events.md`: regenerate after a game update, as with the schema.
- `voltmod/.claude/rules/design.md`: "Players and events".
- `.claude/rules/framework-patterns.md`: "Subscriptions and hooks".
- `voltmod/.claude/skills/gamedata-update/SKILL.md`, next to its schema step.

### Done when

- Every event-driven feature still works:
  - stronghold kill rewards and bounties;
  - bhop's hop boost;
  - anticheat's bullet impacts;
  - admin-system's effect cancels on death and round end.
- Both platforms build.

## Phase 7 - Plumbing and cleanup

- Make these private, with the caller as a friend (`Internal::PluginModule` or `Runtime`):
  - `Runtime::Initialize` and `OnGameFrame`
  - `Map::SetCurrent`
  - `Precache::AddTo`
  - `EntitySystem::Initialize` and `OnServerStartup`
  - `GameEvents::OnServerStartup` and `RemoveAllListeners`
  - `Vote::TryCastBallot`
  - `Visibility::OnCheckTransmit`
  - `ClientConVars::OnClientFullyConnect` and `OnServerStartup`
  - `ChatInput::TryConsume`
  - `CommandManager::Attach`, `HandleChatMessage`, `IsForeign` and `RemoveAll`
- `PlayerManager`'s `Add`, `Remove` and `Clear` and `SlotEvents::Raise` stay public, because the
  SDK-free tests drive the roster and the slot feed through them. admin-system's `~App` replaces
  `Runtime.Players.Clear()` with a loop over `Players.All()` into its own `OnPlayerDisconnect`, which
  is the only handler it needs.
- `ConVars::ExecuteClientCommand(slot, cmd)` becomes `Controller::ExecuteCommand(cmd)`, a verb on the
  wrapper it acts on. The one caller is main-menu's `HubMenu.cpp:139`.
- Delete `Logger<T>` (`App/Logger.hpp`, `tests/App/LoggerTests.cpp`, the Logging part of
  `docs/plugin.md`) and `EntitySystem::AlivePawns()` (and its line in `docs/sdk/entities.md`).
  Nothing uses either.
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

The designs are fixed here so that each one lands the same way when a plugin needs it. None has a
user today.

- **Entity events.** `EntitySystem` gets `Event<Entity> Created`, `Spawned` and `Deleted`. They share
  one lifecycle that adds an `IEntityListener` on the first subscription and removes it after the
  last. It is added again when `OnServerStartup` finds a new entity system.
- **Client command listener.** `CommandManager` gets `Event<ClientCommand&> Received`, raised for
  every command except chat and votes. `ClientCommand` has `Sender` (null for the server), `Name`,
  `Arguments` and `Blocked`; for example, `if (cmd.Name == "jointeam") cmd.Blocked = true;`.
- **Blocking game events.** `GameEvents.Before<T>`, over a hook on `IGameEventManager2::FireEvent`,
  with `Blocked` and `DontBroadcast`. For example, to hide kill-feed entries.
- **User messages.** Block a message type or change its recipients, without per-field protobuf access.
- **Transmit crash guards.** Plugify sends a newly spawned pawn for one tick and always sends dead
  pawns; VoltMod's `Visibility` does neither. If hiding a pawn ever crashes a client, look here first.

## Risks

- **Settings loaded in a member initializer:** a member declared above `Config` cannot read it. When
  the settings fail to load, later members are still built with default settings before the plugin
  is refused, so constructors must not act outside the plugin. Commands and subscriptions are fine,
  because the refusal removes them.
- **The `ClientConnect` hook:** it sits under the SourceHook hooks of legacy Metamod plugins, because
  VoltMod starts first. Test it with the legacy dummy and on Linux.
- **The event generator:** it depends on the `.gameevents` format and on a VPK reader. Commit the
  generated files and regenerate only on purpose, as with the schema.
- **Filtering events without a valid subject:** this could drop an event a plugin wants for a player
  who is disconnecting. Check `player_disconnect` and similar events when the generator lands.
- **Breaking phases:** each phase breaks the API. Every phase updates all five plugins in the same
  change and builds against the editable checkout; nothing is released.

## Out of scope

- Attribute or reflection registration, string-keyed schema or event access, DI containers,
  sync/async twins of each call, and plugin-created convars.
- Per-plugin gamedata or schema files, and generating every schema class. Framework edits cover the
  plugins in this repo, and the manifest keeps a game update from refusing plugins over fields they
  don't use.
- A SAT solver, version requirements beyond `*` and `>=`, `conflicts`, or a package manager for plugins.
- Chrono durations in `Scheduler` (fact 15).
- A hand-written export manifest for other languages. If bindings come, generate them from the C++
  headers.

## Decisions

Made on 2026-09-26:

- Event fields use generated names only; the generator gives three keys a type (phase 6).
- No release: a single `/deploy-test` once all phases are complete, on a new branch with grouped
  commits.
- Translated messages are `SendKey`/`BroadcastKey`; `Send`/`Broadcast` keep sending finished text.
- `Logger<T>` and `EntitySystem::AlivePawns()` are deleted.
- `plugin.json` gains `$schema`, `website`, `license`, `logLevel` and versioned dependencies.

Still open: sending admin-system's `ChatService` broadcasts in each player's language through
`BroadcastKey`. It is planned for later, because those lines are built from several parts.
