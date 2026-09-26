# VoltMod without Metamod

## Goal

VoltMod loads itself and runs whether Metamod is installed or not, in any version.

This change makes VoltMod work with no Metamod at all, and the deploy stops installing Metamod. Once
that is verified, a later change brings back a pinned pre-KHook Metamod for the legacy plugins
(Metamod plugin API 17, SourceHook, some binary-only). The loader already chains to it.

Rules for this change:

- Breaking changes are fine; the framework has no external consumers.
- No fallbacks, no shims, no "also works as a Metamod plugin" mode.
- The smallest design that works. Copy Metamod's proven start-up points instead of inventing new ones.

## Facts this plan relies on

Checked on 2026-09-25 against `references/metamod-source`, `references/plugify-*`, the KHook sources in
the Conan cache and the local `engine2.dll`.

1. **The engine prefers `server_valve`.** On a dedicated server, engine2 calls
   `LoadModule("server_valve", "Source2ServerConfig001", optional)` and loads `server` only when that
   returns nothing. The module is searched on the game binary paths, so a `server_valve` under any
   `Game` line in `gameinfo.gi` wins over every `server`, Metamod's loader included. Plugify's Linux
   autoload uses the same behavior.
2. **Metamod never sees `server_valve`.** Metamod's loader walks the `Game` lines looking only for
   `bin/<platform>/server.dll` / `libserver.so` and skips only its own file. Loaded from our loader, it
   loads the real server itself and treats it as the game DLL, exactly as today.
3. **Metamod's start-up points.** Metamod gets the engine factory in `ISource2ServerConfig::Connect`,
   starts its core and plugins (today's VoltMod host among them) in `ISource2Server::Init` before the
   game's `Init`, and stops them in `ISource2ServerConfig::Disconnect`. It swaps those vtable slots
   directly and writes `Connect` and `Init` back after their first call.
4. **Metamod re-patches `Init`.** Until its core has started, Metamod's `CreateInterface` runs its
   game-DLL request code for every name. Asking it for `Source2Server001` patches `Init` again and
   overwrites the original it saved, so the next `Init` call loops between the two loaders.
5. **KHook needs one owner.** Without `KHOOK_STANDALONE`, KHook's API functions are `inline`
   forwarders to `KHook::__exported__khook`. With it, they are the real implementation (`detour.cpp`,
   `ranges.cpp`, safetyhook, Zydis). One module must own the implementation and must never unload.
   KHook writes a vtable slot once and never writes it back, and its two worker threads must be
   joined with `KHook::Shutdown()` before exit.
6. **The first `Game` line is the write path.** The engine writes files such as `console.log` under the
   first `Game` line; that is why the local log sits in `addons/metamod/` today.
7. **Build 1411 is the last pre-KHook Metamod.** The mirror has `mmsource-2.0.0-git1411` for Windows and
   Linux (commit `7ec0f16`, 2026-08-31). The next build, 1459, is the KHook merge.

## Design

### Files on a server

```text
csgo/gameinfo.gi
    Game  csgo/addons/metamod     only where Metamod is installed; the deploy no longer adds it
    Game  csgo/addons/voltmod     always, directly above `Game csgo`
    Game  csgo
csgo/addons/voltmod/bin/<platform>/
    server_valve.dll | libserver_valve.so    the loader: new
    voltmod.dll | voltmod.so                 the host: no longer a Metamod plugin
```

`addons/metamod/voltmod.vdf` goes away. Everything VoltMod ships stays under the instance's own
`addons/`, which matters on Docker hosts, where instances share one CS2 install and mount only
`addons/`.

### Start-up and shutdown

1. The engine finds `server_valve` in `addons/voltmod/bin/<platform>/` and calls its
   `CreateInterface("Source2ServerConfig001")`.
2. On that first call the loader:
   - returns null without `-dedicated`, so it can never run inside a player's game client; the engine
     then loads `server` itself.
   - reads `gameinfo.gi` and loads the first `bin/<platform>/server` on the `Game` lines. That is
     Metamod's loader where Metamod is installed and the game's own server otherwise.
   - forwards this and every later `CreateInterface` call to that module.
3. On the `Source2ServerConfig001` object it forwards, the loader replaces `Connect` (keeps the engine
   factory) and `Disconnect`. On the `Source2Server001` object, it replaces `Init`, and records the
   game's own server factory: `CreateInterface` of the module that owns that object's vtable (fact 4).
4. `Init` loads the host from the loader's directory, calls `VoltMod_HostStart`, then calls the saved
   `Init`. With Metamod present, that saved `Init` is Metamod's, so Metamod and the legacy plugins start
   after VoltMod. This is the same point where Metamod starts the host today, so timing does not change.
5. At shutdown, Metamod's `Disconnect` runs first because it patched later; it unloads the legacy
   plugins and calls ours. Ours calls `VoltMod_HostStop`, then `KHook::Shutdown()`, then the saved
   `Disconnect`.

The three replacements are plain slot swaps that are never written back, not KHook hooks. Metamod
writes `Connect` and `Init` back after the first call, which would drop a KHook thunk. `Disconnect`
calls `KHook::Shutdown()`, which must not run inside a KHook callback. The replacement functions are
free functions that take the object first. On x64 both ABIs pass `this` as the first argument, and
none of the three returns a struct.

VoltMod starting first also fixes the hook-order problem: its engine hooks sit underneath the legacy
plugins' SourceHook hooks, so a legacy plugin that unloads restores VoltMod's hook instead of wiping it.

### Who owns what

| Module | Owns |
| --- | --- |
| Loader (`src/Loader/`) | `CreateInterface`, the `gameinfo.gi` lookup, the three slot swaps, the KHook implementation and the `IKHook` dispatcher it hands to the host. Never unloads, so KHook's JIT code lives as long as the process. |
| Host (`src/Host/`) | `VoltMod_HostStart` / `VoltMod_HostStop`. Everything `MetamodEntry::Load` does today. |
| `HostStart` (`src/Host/HostStart.hpp`) | The one struct both sides share: engine factory, game server factory, `KHook::IKHook*`, `csgo` directory. The loader and host always ship together, so there is no version check. |

## Changes in the framework (`voltmod/`)

### 1. KHook package

- Replace `recipes/metamod-source` with `recipes/khook`: `alliedmodders/khook` at the commit Metamod
  `fa6f80e` pins, with its safetyhook submodule. Fetch Zydis in `source()`, not during the build.
- Two components:
  - `khook::headers`: `khook.hpp` only. The SDK links this, and so every plugin gets it.
  - `khook::khook`: the static implementation built with `KHOOK_STANDALONE`, carrying that define.
    Only the loader links it.
- `conanfile.py`: require `khook` instead of `metamod-source`; the `sdk` component requires
  `khook::headers`; drop "Metamod:Source" from the description and the comment at line 90.
- `CMakeLists.txt`: `find_package(khook)`; `voltmod-sdk` links `khook::headers` in place of
  `VoltMod::Metamod`; the PCH takes `<khook.hpp>` instead of `<ISmmPlugin.h>`.
- Rename `metamod-source` to `khook` in `tools/release/sdk_updates.py`, `tools/release/conan_packages.py`,
  `cli/voltmod/toolchain/conan.py` (`PREBUILT_SDK_ARGS`) and cs2-plugins'
  `.claude/skills/deploy-test/scripts/linux_build.py` (`OWN_RECIPES`). Upload Windows and Linux binaries
  like the other own recipes; the package is no longer header-only.

### 2. Loader (new, `src/Loader/`)

- `Loader.cpp`:
  - The exported `CreateInterface`, the next-module lookup and forwarding, the `Connect` / `Init` /
    `Disconnect` swaps, and loading and calling the host.
  - Logs through tier0's `Msg` / `Warning` with a `[VoltMod]` prefix.
  - On any failure before forwarding (next module missing, not dedicated), it logs and returns null.
  - If the host fails to start, `Init` logs the reason and still calls the saved `Init`, so the server
    runs without VoltMod, the way a failed Metamod plugin load behaves today.
- `GameInfo.hpp/.cpp`: `std::vector<std::string> GameSearchPaths(std::string_view gameinfo)` returns the
  `Game` values inside `SearchPaths`, in order. It ignores other keys (`Game_LowViolence`, `Mod`,
  `Write`...), comments and quotes. The loader resolves each value against the `game/` directory, four
  levels above its own file.
- `HookDispatcher.hpp`: `HookDispatcher final : KHook::IKHook`, forwarding each method to the
  standalone `KHook::` function. This is Metamod's `KHookImpl` without the per-plugin bookkeeping,
  because VoltMod's `Subscription`s remove their own hooks.
- The game server factory: `GetModuleHandleExA(FROM_ADDRESS)` on Windows, `dladdr` + `dlopen(RTLD_NOLOAD)`
  on Linux, on the `Source2Server001` object's vtable, then `CreateInterface` from that module.
- CMake:
  - `voltmod_add_module(voltmod-loader ...)` with `INSTALL_DIR "addons/voltmod/bin/${VOLTMOD_BIN_SUBDIR}"`
    and `COMPONENT host`.
  - `OUTPUT_NAME "server_valve"` and `PREFIX "${CMAKE_SHARED_MODULE_PREFIX}"`, which gives
    `libserver_valve.so` on Linux, the name the engine looks for.
  - Links `khook::khook` and `VoltMod::HL2SDK`.
  - On Linux, link with `-Wl,--exclude-libs,ALL` so only `CreateInterface` is exported, and KHook,
    safetyhook and Zydis symbols cannot bind to another module's copy.
- Layering lint: add `Loader -> Host` (it includes only `Host/HostStart.hpp`).
- `AllowDedicatedServers`: not replaced. Metamod forces it to true on Windows; step 1 of the rollout
  checks whether the local Windows server needs that without Metamod, and it is added only if it does.

### 3. Host

- Delete `src/Host/MetamodEntry.hpp/.cpp` and `cmake/host.vdf.in`, and the `configure_file` / `install`
  of `voltmod.vdf` in `CMakeLists.txt`.
- Add `src/Host/HostEntry.cpp`:
  - Exports `extern "C" bool VoltMod_HostStart(const HostStart*, char* error, size_t errorSize)` and
    `extern "C" void VoltMod_HostStop()`.
  - The body is today's `Load` / `Shutdown` minus `PLUGIN_*`, the `late` flag and the Metamod API.
  - It defines the host's own `KHook::__exported__khook` and sets it from `HostStart`.
  - The gamedata comment "Another Metamod plugin may already hold a slot" becomes "a legacy plugin
    may already hold a slot".
- `PluginHost` keeps a `HostStart` in place of the `ISmmAPI*`. `EngineHooks::Install()` and
  `SchemaService::Initialize()` resolve through it instead of taking `ISmmAPI*`.
- `Host/EngineInterfaces.hpp` keeps `ResolveInterface`; the two Metamod lambdas go.

### 4. Plugin-facing API (breaking; `HostAbiVersion` 3 -> 4)

- `IHost`:
  - Remove `Metamod()`.
  - Add `void* EngineInterface(const char* version) const`, `void* ServerInterface(const char* version) const`
    and `std::string_view BaseDir() const`, all implemented in `HostView` from the `HostStart`.
  - `HookDispatcher()` stays.
- `src/Runtime.cpp`:
  - `InstallLogger` calls `SetBaseDir(host->BaseDir())`.
  - `ResolveInterfaces` resolves through `EngineInterface` / `ServerInterface`.
  - The two `ismm->Format` calls become `std::format_to_n` into `context.Error`, null-terminated.
- `Engine/EngineTypes.hpp`: drop the `SourceMM::ISmmAPI` forward declaration; describe `KHook::IKHook`
  as the loader's dispatcher.
- `Engine/Detours.hpp`: include `<khook.hpp>` instead of `<ISmmPlugin.h>` and reword the comment (the
  host fills its pointer from the loader).
- Comments that name Metamod: `Engine/Interfaces.hpp:11`, `Players/PlayerManager.hpp:85`,
  `Core/Files/Paths.hpp:11`.
- `App/PluginEntry.hpp` and `src/App/Plugin.cpp` do not change: plugins still seed their
  `__exported__khook` from `IHost::HookDispatcher()`.

### 5. CLI (`cli/voltmod/`)

- `server/install.py`: drop `HOST_VDF` and the Metamod wording. The loader is in the host component
  under `addons/`, so the existing merge already installs it.
- `server/cs2_server.py`: `METAMOD_SEARCH_PATH` becomes `VOLTMOD_SEARCH_PATH = "csgo/addons/voltmod"`.
  The has/restore pair is renamed, and restore inserts the line directly above `Game csgo`.
- `server/launch.py`: restore VoltMod's line before launching.
- `checks/doctor.py`: check for the loader file and VoltMod's `gameinfo.gi` line, not for Metamod.
- `cli.py`: help text.

### 6. Tests

- `tests/Loader/GameInfoTests.cpp`: `GameSearchPaths` on a real `gameinfo.gi` excerpt, with and without
  Metamod's line. It covers order, `Game_LowViolence`, comments and quotes. The test target compiles
  `src/Loader/GameInfo.cpp` directly (SDK-free). This is the only new test.

### 7. Docs

- Replace "a Metamod plugin" with "loads itself through `server_valve`", and drop Metamod from the
  install steps, in these files:
  - `docs/`: `host.md`, `architecture.md`, `getting-started.md`, `consuming-via-conan.md`, `testing.md`,
    `plugin.md`, `mainpage.md`, `framework-comparison.md`, `sdk/gamedata.md`.
  - Repo root: `README.md`, `CLAUDE.md` (intro and the `recipes/` line), `pyproject.toml`, `Doxyfile`.
  - `templates/project/README.md` and `templates/project/pyproject.toml`.
  - `.claude/skills/gamedata-update/SKILL.md`.
- `CHANGELOG.md`: one breaking entry: the new install layout, `IHost` changes, and the Metamod
  requirement removed.

## Changes in cs2-plugins

### 8. Deploy

- **Metamod leaves the deploy.** VoltMod no longer needs it, and the legacy plugins come later.
  - Delete `deploy/panel/metamod.py` and its use in `deploy/panel/deployer.py` (the read, the plan
    line, `download` and `install`).
  - `files/docker/pre.sh` drops the Metamod install and the `MMS_BASE` / `MMS_URL` handling.
  - Reword the comments that mention Metamod: `deploy/panel/api.py:16` (the mirror) and
    `deploy/docker/ssh.py:29`.
  - Servers keep whatever Metamod they already have; the deploy just stops touching it.
- **`gameinfo.gi`:** `deploy/panel/gameinfo.py` and `pre.sh` add `Game csgo/addons/voltmod` directly
  above `Game csgo` instead of Metamod's line. Other lines, Metamod's included, are left alone.
  Update the comment in `files/panel/gameinfo.gi`.
- **The host check:** `deploy/bundle/builder.py` looks for `voltmod/bin/linuxsteamrt64/libserver_valve.so`
  instead of `metamod/voltmod.vdf`, and `pre.sh` stops the container on the same file.
- **`deploy/README.md`:** the layout tree, the "Metamod loads the host" paragraph, the panel and Docker
  steps (no Metamod download or install), and the troubleshooting rows at lines 305-309.

### 9. Repo text and skills

- `CLAUDE.md` line 3, `README.md` (install steps; `meta list` becomes `volt list`), the `conanfile.py`
  comment, the `pyproject.toml` description, `plugins/admin-system/README.md` requirements,
  `docs/local-development.md`.
- `.claude/rules/framework-patterns.md:11`.
- `.claude/skills/rcon-debug`: `scripts/local.py` restores VoltMod's line. Without Metamod, the local
  log moves to `game/csgo/addons/voltmod/console.log` (fact 6); update `SKILL.md` to match.

### 10. Dummy legacy plugin (`tools/legacy-dummy/`)

The real legacy plugins are with the partner, so a stand-in built the same way tests the legacy path.
It is a Metamod plugin built against the pre-KHook headers (plugin API 17) that hooks, through
SourceHook, the same engine functions VoltMod hooks.

- **Source:** one `legacy_dummy.cpp`, cut down from Metamod's own `samples/s2_sample_mm` at `7ec0f16`:
  - `SH_DECL_HOOK` / `SH_ADD_HOOK` on `IServerGameDLL::GameFrame` (post),
    `IServerGameClients::ClientCommand` (pre) and `IServerGameClients::ClientPutInServer` (post).
    Each counts its calls and lets the call through; `SH_REMOVE_HOOK` on unload.
  - A `legacy_dummy_status` console command that prints the three counts.
  - Load and unload lines through `META_CONPRINTF`.
- **Build:** its own CMake project outside the main build (the root `CMakeLists.txt` names its plugins,
  so `tools/` is never picked up). It reuses the main build's Conan output, so there is no second
  Conan install.
  - `find_package(hl2sdk-cs2)`, link `VoltMod::HL2SDK`, and call `hl2sdk_attach_plugin_support` for
    `memoverride.cpp` and `convar.cpp`, as Metamod's sample does. The package already defines
    `META_IS_SOURCE2` and the engine macros.
  - Metamod's headers at `7ec0f16` through `FetchContent`; include `core/` and `core/sourcehook/`.
  - Windows, in the MSVC dev shell:
    `cmake -S tools/legacy-dummy -B build/legacy-dummy -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=build/windows-msvc-release/generators/conan_toolchain.cmake`,
    then `cmake --build build/legacy-dummy`.
  - Linux: the same two commands with `build/linux-steamrt-release/generators/`, inside the CI
    container that `/deploy-test` builds in.
- **Install by hand, like a real legacy plugin:** Metamod git1411 into `addons/metamod/`, the plugin at
  `addons/legacy_dummy/bin/<platform>/`, and `addons/metamod/legacy_dummy.vdf` naming it.
- Add `tools/` to the layout in `CLAUDE.md`.

## Rollout

Testing needs no framework release: cs2-plugins builds against the editable `voltmod` checkout, and
`/deploy-test` builds the checkout in the CI container and pushes it straight to a server.

0. **Check the legacy baseline first.** Build the dummy (step 10) and run it on the local server under
   Metamod 1411, with VoltMod's `.vdf` removed. `meta version` shows 1411, `meta list` shows the
   dummy, and `legacy_dummy_status` counts rise. This needs none of the loader work, and it shows early
   whether build 1411 still runs on the current CS2 build.
1. **Prove the loader locally.** Build steps 1-5 and run on the local Windows server in three setups:
   - No Metamod: move `addons/metamod` aside first. The server starts, the log shows the loader and
     the host, `volt list` shows the plugins, stronghold and main-menu work, and `quit` exits cleanly.
     If the server refuses to start as dedicated, add the `AllowDedicatedServers` swap on Windows.
   - The current (KHook) Metamod still installed, as on today's servers: the same checks, with two
     KHook copies in the process.
   - Metamod 1411 and the dummy:
     - `meta list` shows the dummy, `volt list` shows ours, and the log shows VoltMod's host
       starting before Metamod loads the dummy.
     - With a client joined, `!menu` works and the dummy's counts rise: both hook engines see the
       same calls.
     - `meta unload` the dummy: `!menu` and the HUD still work, because VoltMod's hooks sit
       underneath. `meta load` it back and the counts rise again.
     - `quit` exits cleanly, with no crash dump.
2. **Prove it on Linux.** Make the deploy changes (step 8), then push to one test server with
   `/deploy-test`, or copy the built `addons/voltmod` by hand. Repeat the three setups: first with
   Metamod moved aside, then with Metamod 1411 and the Linux build of the dummy copied in by hand.
3. **Finish the rest.** Tests, docs and repo text (steps 6-7 and 9).
4. **Clean up servers that keep Metamod.** Delete the stale `addons/metamod/voltmod.vdf` by hand (on
   Docker hosts, `instances/<name>/addons/metamod/voltmod.vdf`). The sync never deletes, and Metamod
   would otherwise try to load the host as a plugin.

## Later

- **Legacy plugins.** Once VoltMod is verified, add a pinned pre-KHook Metamod back to the deploy:
  the panel install, `MMS_URL` for Docker, and Metamod's `gameinfo.gi` line above VoltMod's. Pin the
  build the partner's servers run; 1411 is the newest one that can load them (fact 7). Then install
  the partner's plugins on a test server and repeat the dummy checks from rollout step 1 with them.
- **Release.** Release the framework once everything is verified, then relock cs2-plugins; the
  `voltmod` range in `conanfile.py` follows the new version.

## Risks

- **`server_valve` is undocumented.** If Valve changes it, VoltMod stops loading; there is no fallback
  by design. After each CS2 update, confirm the host still loads (`volt list`) as part of the
  gamedata-update run.
- **`AllowDedicatedServers` on Windows:** rollout step 1 answers it.
- **Late hooks:** a hook VoltMod adds after start-up can still sit above a legacy plugin's SourceHook
  hook. Restart the server instead of `meta unload` for legacy plugins, as the deploy already does.
- **Shutdown crash:** `KHook::Shutdown()` at `Disconnect` mirrors Metamod, including its known shutdown
  crash fixes. Watch the `quit` check in step 1.

## Out of scope

- Porting the legacy plugins, or forking Metamod.
- Any Metamod-plugin mode for the host, or a `gameinfo.gi`-free install into `csgo/bin`.
- Plugify's launcher mode, which would change every host's start command.
