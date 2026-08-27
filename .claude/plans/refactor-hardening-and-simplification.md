# VoltMod API v2 hardening and simplification plan

## Purpose

Finish the refactor/api-v2 work in the two repositories without reverting the parts that improved ownership, safety, and API clarity.

The work has three goals, in this order:

1. Fix correctness and lifetime defects discovered during review.
2. Remove machinery that has no consumer in this project.
3. Reduce build, tooling, and documentation overhead without weakening the new safety boundaries.

This repository is the only VoltMod consumer. Compatibility with unknown third-party consumers is therefore not a constraint. Compatibility between the checked-out VoltMod framework and the plugins in this repository is required.

## Repository boundaries

Treat the repositories as two independent worktrees:

- Plugin repository: repository root.
- Framework repository: vendor/voltmod.

Keep their commits separate. Framework commits should not include the parent repository's lockfile or plugin migrations. Parent-repository commits should not include changes inside vendor/voltmod.

Avoid rebuilding the Conan package and refreshing the parent lockfile after every framework edit. Complete and verify coherent framework phases locally first. Create the package and refresh the parent lockfile at an integration checkpoint, then migrate and verify the plugins.

## Amendments from the 2026-08-26 review

Every claim in Phase 1 was verified TRUE against the source. These amendments supersede the
text below where they conflict. Full reasoning:
`C:\Users\admin\.claude\plans\review-this-plan-i-playful-pony.md`.

1. ~~New Phase 0 - find the out-of-bounds write.~~ **Dropped.** The static audit found no live
   bug (see the Phase 0 note), and the dynamic hunt was judged not worth the effort. Keep the
   canary; if a load-time crash appears after a header change, check `sizeof(Runtime)` first.
2. **Re-sequenced.** Delete before you fix, and put CI first: 2 -> 0 -> 3 -> 1 -> 4 -> 5 -> 6
   -> 7 -> 8 -> 9. The framework's 294 test cases are never compiled or run by its own CI
   (`conanfile.py:108-109` forces `BUILD_TESTING=False`), so Phase 2 gates everything after it.
3. **Four factual errors fixed inline** in 3.2, 4.2 and 5.7 - `Field` `|=`/`&=`/`==`,
   `JsonConfig::Mutable`, `Players/EffectDispatcher.hpp`, and the BOM count.
4. **StatusService is a live bug, not a comment fix.** Move 4.3 into Phase 1: `:37` never
   checks `is_discarded()`, so one malformed provider makes the whole `STATUS_JSON` line
   invalid JSON - the line advertised as machine-readable for RCON tooling.
5. **1.6 is privilege escalation.** Typing an immune admin's SteamID64 at `!ban` silently
   becomes an offline-SteamID ban that bypasses `Policy::CanTarget`. Rank it with 1.1.
6. **1.7's minimal solution is insufficient.** Plain `Button(..., Allowed("s"))` rows get no
   runtime re-check at all - the pattern `docs/menu.md:75-79` recommends and
   `AdminMenu_Control.cpp:108,113-118,134-141` uses. Fixing docs alone leaves those enforcing
   a build-time snapshot.
7. **Phase 7 rule reversals - mostly dropped.** Keep the anonymous-namespace ban (314
   file-scope statics stand in for it; relaxing deletes nothing) and the using-directive ban
   (zero violations today). For forward declarations, fix the tooling rather than the policy:
   `modgraph.py:78` exempts any `\w*Types\.hpp$` by regex and only fires on headers, while both
   CLAUDE.md files ban it globally. Name the exempt files explicitly and correct the docs to
   say "in headers".
8. **4.1 magic_enum - middle path.** `Core/Capabilities.hpp:3` also includes `EnumNames.hpp`
   and `Runtime.hpp` pulls it in, so magic_enum reaches every TU regardless of `Api.hpp`. Add a
   `Count` sentinel to `Capability`, drop the include from both `Capabilities.hpp` and
   `Api.hpp`, add direct includes to the 6 `.cpp` files that call `Name()`, delete the unused
   `Parse<E>()`. No hand-written switches.
9. **3.3 - investigate before deleting.** The dependency warning
   (`PluginIdentity.cpp:13-38`, identity-key lookup) and the functional fallback
   (`ResponseManager.cpp:101-106`, `IAdminActions` via `Exchange.Get<>`) are independent paths.
   Determine which is broken first; if `IAdminActions` also returns null, anticheat has been
   running without admin-system integration.
10. **Make the unsafe call unrepresentable** rather than fixing each site. In 1.4, delete the
    `int slot` overloads of `ActionDispatcher`/`EffectDispatcher` and `PlayerManager::RefFor`
    outright instead of keeping thin adapters - then a row callback cannot capture an int. In
    1.1, stop exposing `Resolution::Address` and give it one accessor that returns null unless
    `Error.empty() && Address && Unique`.
11. **Smaller wins:** unify the two divergent `ParseInt64` helpers into one
    `Strings::ParseInt<T>()` using `std::in_range` (closes 1.8 and the duplication together);
    generate the module layering table from `modgraph.py:10-38` rather than keeping hand
    copies; add Python tests for `modgraph.py` (305 lines, gates every PR, untested); delete
    `Hooks::Damage` first and in isolation as a cheap OOB-write probe.
12. **Fold Phases 6 and 8 into the phases that touch the code.** Each phase updates its own
    tests and docs in the same commit; Phase 8 shrinks to the measured duplication (~18
    Subscription-ownership restatements, 3 copies of the layering table, 2 of the game-event
    list, 3 of the Conan block).
13. **Checkpoint after Phase 3.** Re-read the remaining scope with the deletions banked before
    committing to the style sweeps in 5.7-5.12.

## Execution checklist

- [x] Record both repository baselines and run the existing verification commands.
- [x] Phase 2: make VoltMod CI run CTest.
- [x] Phase 0: dropped after a static audit - no live bug, see the phase note.
- [x] Phase 3: remove confirmed unused surface. Done 2026-08-26; ~870 lines out.
- [ ] Phase 1: fix correctness and lifetime blockers (now including 4.3 StatusService).
- [ ] Checkpoint: re-scope 5.7-5.12 and Phase 7 with the deletions banked.
- [ ] Phase 4: reduce public include and dependency cost.
- [ ] Phase 5: simplify plugin-side boilerplate.
- [ ] Phase 6: simplify tests and test seams carefully.
- [ ] Phase 7: simplify modgraph and repository rules.
- [ ] Phase 8: consolidate documentation.
- [ ] Phase 9: package, integrate, and complete all verification.

Claude Code should execute one numbered phase at a time. At the end of a phase:

- Run that phase's focused tests plus the repository-wide test and lint gates.
- Inspect the diff for unrelated formatting or generated-file churn.
- Update this checklist and add a short progress note under the phase.
- Do not continue past a failing gate.
- Do not silently choose a behavior-changing deletion where this plan identifies a decision point.

## Baseline and invariants

Before implementation:

- Record git status and HEAD in both repositories.
- Confirm both worktrees are clean or identify pre-existing user changes.
- Run the framework tests and lint:
  - cd vendor/voltmod
  - uv run poe test
  - uv run poe lint
  - uv run poe modgraph
- Run the plugin tests and lint:
  - cd ../..
  - uv run poe test
  - uv run poe lint

Preserve these architectural decisions:

- Event lifecycle arming and Subscription ownership.
- Scheduler timers cancel when their Subscription drops.
- PlayerRef is the stored cross-callback identity.
- ActionContext contains the authorized pair and frame-local entity wrappers, not Runtime.
- Policy::Authorize remains the single permission and immunity gate.
- ConVar handles remain typed.
- GameData and Bindings fail closed before raw addresses, vtable slots, or offsets are used.
- The real module dependency DAG remains mechanically enforced.
- API-surface compile tests remain.
- Result and Status remain based on std::expected.

Explicit non-goals:

- Do not add detached or fire-and-forget scheduler callbacks.
- Do not put Runtime& back into ActionContext or lower framework modules.
- Do not resolve bool convars through ConVar<float>.
- Do not broadly remove gamedata validation based on line count.
- Do not replace API-surface tests with the Conan test package.
- Do not introduce an AppPlugin<T> inheritance layer.
- Do not flatten WorldServices or HookServices merely to reduce one level of member access.

## Phase 0: the out-of-bounds write - dropped

Static audit only; the dynamic repro was cancelled as not worth the effort.

What the audit established: `PerSlot<T>::operator[]`
(`include/VoltMod/Core/PerSlot.hpp:45-52`) bounds-checks with `assert`, which compiles out
under NDEBUG, and `windows-msvc-release` is the only preset that links. But **every current
caller validates with `IsValidSlot` first** - `MenuManager::_states` (its unguarded indexers
are private and reached only from `OnGameFrame`'s bounded loop), anticheat's `_dumpTicks`,
`_sim`, `_lastTeleport`, `_pendingKick`, and admin-system's `_pendingNotice`, `_pendingKick`.
So there is no live bug here, only a convention. A hardening attempt was reverted as
over-engineering.

The Runtime canary stays as-is. Treat the layout sensitivity as a known hazard: if a load-time
crash appears after a header change, check whether `sizeof(Runtime)` moved before chasing the
reported crash site.

## Phase 1: correctness and lifetime blockers

Complete this phase before dead-code deletion. Add regression tests first where the affected code is SDK-free.

### 1.1 Make ambiguous gamedata unusable

Problem:

- GameData records a first address even when a pattern is not unique.
- FailureSummary reports the ambiguity, but Bindings checks only Error.
- Capabilities can remain enabled and raw calls can use an arbitrary match.

Primary files:

- vendor/voltmod/src/Engine/GameData.cpp
- vendor/voltmod/src/Engine/Bindings.cpp
- vendor/voltmod/include/VoltMod/Engine/GameData.hpp
- vendor/voltmod/tests/Engine/BindingsTests.cpp
- vendor/voltmod/tests/Engine/GameDataFileTests.cpp, if shared fixtures belong there

Implementation:

- Define one authoritative usability predicate for a GameData::Resolution.
- A signature or derived address is usable only when:
  - Error is empty.
  - Address is non-null.
  - Unique is true.
- EntryError must report an ambiguous pattern as an error suitable for a capability reason.
- AddressOf must return null for an ambiguous resolution.
- ResolveAddress must not derive a rel32 target from an ambiguous source signature.
- BindVTable and IndexOf should keep their existing section-specific checks; Unique is meaningful only for scanned signatures and addresses.
- Preserve FailureSummary as diagnostics, but do not rely on it for safety.

Tests:

- An ambiguous signature disables every capability that requires it.
- AddressOf-equivalent binding leaves the typed function unbound.
- A derived address from an ambiguous signature remains unbound.
- FailureSummary still names the ambiguous entry.

Acceptance:

- No raw function pointer or derived address is populated from a non-unique scan.
- Runtime may continue in degraded mode, but the affected capability is off.

### 1.2 Make GameDataFile::Parse non-throwing for all input shapes

Problem:

- Only JSON syntax parsing is caught.
- value(), items(), and nested object access can throw type_error for structurally invalid JSON.
- The exception can escape Runtime::Start instead of becoming Error::Invalid.

Primary files:

- vendor/voltmod/src/Engine/GameDataFile.cpp
- vendor/voltmod/src/Engine/GameDataFile.hpp
- vendor/voltmod/tests/Engine/GameDataFileTests.cpp

Implementation:

- Check that build and each top-level section have the expected object type before iterating or calling value().
- Check that every section entry and platform-specific entry has the expected object or integer type.
- Return a path-specific malformed error, such as build is not an object or signatures.X.windows is not an object.
- Add a final conversion-exception boundary around the complete structural parse as defense in depth.
- Do not remove format versioning, duplicate-key checks, platform checks, signature-pattern validation, address-to-signature validation, vtable bounds, or offset validation in this phase.
- Ensure Load adds the source path to every returned parse error.

Tests:

- build as an array.
- signatures as a scalar.
- a signature entry as an array.
- a platform signature entry as a string.
- addresses, vtables, and offsets with wrong section and entry types.
- wrong numeric types for max, align, rel32At, and platform values.
- Every case returns ErrorCode::Invalid and does not throw.

Acceptance:

- GameDataFile::Parse is total over arbitrary JSON text: it returns a Result and never leaks a nlohmann conversion exception.

### 1.3 Fix custom hook ownership and shutdown order

Problem:

- MetamodPlugin::Shutdown clears only standard hooks.
- AdminSystemPlugin::OnUnload destroys _app while _clientListening remains installed.
- A failed OnLoad can leave an active hook whose callback dereferences an empty _app.

Primary files:

- vendor/voltmod/include/VoltMod/App/MetamodPlugin.hpp
- vendor/voltmod/src/App/MetamodPlugin.cpp
- plugins/admin-system/src/Core/Plugin.hpp
- plugins/admin-system/src/Core/Plugin.cpp
- vendor/voltmod/docs/plugin.md
- vendor/voltmod/docs/sdk/hooks.md
- templates, if they document or demonstrate custom hooks

Preferred design:

- Add a protected MetamodPlugin helper that accepts and owns a custom hook Subscription.
- Store custom hook subscriptions in a base-owned collection distinct from standard hooks.
- OnRegisterHooks implementations register through that helper instead of storing hook subscriptions in derived members.
- Shutdown order:
  1. Clear custom hooks so callbacks into plugin-owned state stop.
  2. Call OnUnload while the Runtime is still alive.
  3. Clear standard hooks.
  4. Destroy Runtime.
- Use the same path after OnLoad failure and normal Unload.
- Keep Shutdown idempotent.

Admin-system migration:

- Remove _clientListening from AdminSystemPlugin.
- Register the SetClientListening hook through the base helper.
- Keep the callback's _app access guarded if an empty state is still representable.

Tests or verification:

- If MetamodPlugin is practical to unit-test, use fake Subscriptions to assert teardown ordering.
- Otherwise extract the small subscription owner into an SDK-free helper with an ordering test.
- Manually exercise successful load/unload, meta reload, and an intentionally failed OnLoad.

Acceptance:

- No custom hook remains installed after Shutdown returns.
- Failed loads cannot leave a callback into empty plugin state.

### 1.4 Preserve PlayerRef through menu dispatch

Problem:

- MenuBuilder::For accepts PlayerRef but context rows capture only Slot.
- ActionDispatcher reconstructs a new reference for the current slot occupant.
- An old menu can act on a different player after target disconnect and slot reuse.

Primary files:

- vendor/voltmod/include/VoltMod/Players/ActionDispatcher.hpp
- vendor/voltmod/src/Players/ActionDispatcher.cpp
- vendor/voltmod/include/VoltMod/Players/EffectDispatcher.hpp
- vendor/voltmod/src/Players/EffectDispatcher.cpp
- vendor/voltmod/src/Menu/MenuBuilderRows.cpp
- vendor/voltmod/include/VoltMod/Menu/MenuBuilder.hpp
- vendor/voltmod/tests/Players/PolicyAuthorizeTests.cpp
- vendor/voltmod/tests/Menu/MenuRenderTests.cpp or a new SDK-free menu dispatch test
- plugins/admin-system/src/Admin/Menu/AdminMenu_Control.cpp
- plugins/admin-system/src/Admin/Menu/AdminMenu_Effects.cpp

Implementation:

- Make PlayerRef the primary ActionDispatcher Resolve and Run input.
- Resolve both references through PlayerManager::Get(PlayerRef) at activation time.
- Reject stale caller or target references without retargeting.
- Carry PlayerRef through EffectDispatcher.
- Context-row callbacks capture PlayerRef values, not integer slots.
- BuildEffectPicker resolves target with Players.Get(target), not Players.Get(target.Slot).
- Keep slot overloads only for genuinely immediate call sites. If retained, make them thin adapters that create references at the caller boundary and document that they are not for stored callbacks.
- Consider closing the current viewer's menu when a captured caller or target reference is stale.

Tests:

- Build a row for target A.
- Remove A, add B in the same slot, activate the old row.
- Verify no action or effect runs on B.
- Repeat for a recycled caller slot.
- Verify a still-live reference continues to dispatch normally.

Acceptance:

- Stored menu actions cannot cross a slot-generation boundary.

### 1.5 Make ConVar<T> compatibility lossless

Problem:

- ConVar<int> accepts UInt32, Int64, and UInt64 and narrows them to int.
- ConVar<float> accepts Float64 and narrows it to float.
- The public typed-handle promise is stronger than the implementation.

Primary files:

- vendor/voltmod/src/Engine/ConVarTypes.cpp
- vendor/voltmod/src/Engine/ConVarTypes.hpp
- vendor/voltmod/src/Engine/ConVars.cpp
- vendor/voltmod/include/VoltMod/Engine/ConVars.hpp
- vendor/voltmod/tests/Engine/ConVarTests.cpp

Implementation:

- bool matches Bool only.
- float matches Float32 only.
- int matches Int16, UInt16, and Int32 only, assuming int is at least 32 bits as required by the supported toolchains.
- Reject UInt32, Int64, UInt64, and Float64 rather than narrowing.
- Do not add exact-width public handle specializations until a real consumer needs them.
- Update documentation that currently calls bool, int, float, and string the engine's exact types.

Tests:

- Confirm every lossless match.
- Confirm every narrowing match is rejected.
- Keep the bool-versus-int regression case.

Acceptance:

- A successful ConVar<T>::Find guarantees Get does not narrow the engine value.

### 1.6 Restrict PlayerOrSteamId fallback

Problem:

- A numeric token falls back to an offline SteamID after any target-resolution failure.
- That includes immunity or other policy rejection, not only no online match.

Primary files:

- vendor/voltmod/src/Commands/CommandRouter.cpp
- vendor/voltmod/src/Commands/Targeting.hpp
- vendor/voltmod/tests/Commands/CommandRouterTests.cpp
- plugins/admin-system/src/Commands/FreezeCommands.cpp

Implementation:

- Preserve the TargetFailure long enough to inspect its TargetError.
- Fall back to a bare SteamID only for TargetError::NoMatch.
- Return immune, ambiguous, or invalid-selector failures normally.
- Retain the explicit hierarchy check in freeze_admin as defense in depth.

Tests:

- Unmatched numeric ID binds as offline.
- Online target binds as online.
- Online immune numeric target returns target.immune and does not become offline.
- An ambiguous or otherwise rejected numeric selector does not fall back.

Acceptance:

- The hybrid argument type cannot erase an authorization failure.

### 1.7 Correct the menu permission contract

Problem:

- Documentation says Policy::Authorize is checked on every render and press.
- Allowed() is evaluated once when the option is built.
- Context actions do reauthorize when pressed, but the visual enabled state is a snapshot.

Preferred minimal solution:

- Do not add a dynamic enabled callback to every menu option in this cleanup.
- Update MenuBuilder documentation to say:
  - Allowed() controls initial enabled state.
  - Context actions reauthorize on activation.
  - A stale visual state may remain until the menu is rebuilt.
- Ensure every context action and effect activation actually passes through ActionDispatcher after the PlayerRef changes above.
- If live permission rendering is a product requirement, implement it later as a separate menu feature with focused tests.

Acceptance:

- Documentation matches behavior.
- Permission revocation cannot execute an action, even if the row remains visually enabled.

### 1.8 Reject out-of-range numeric narrowing

Problem:

- ParseTargetToken accepts every non-negative int64_t after `#` and narrows it to int.
- A value above INT_MAX can therefore become an implementation-defined slot before roster lookup.
- CommandRouter performs the same int64_t-to-int contract with a verbose manual min/max check.

Primary files:

- vendor/voltmod/src/Commands/Targeting.cpp
- vendor/voltmod/src/Commands/CommandRouter.cpp
- vendor/voltmod/tests/Commands/TargetingTests.cpp
- vendor/voltmod/tests/Commands/CommandRouterTests.cpp

Implementation:

- Use `std::in_range<int>` before narrowing a parsed slot or integer command argument.
- Continue to reject negative slots.
- Keep semantic bounds separate from representability: target resolution may still report no match for an int-valued slot that is outside the live roster.
- Use `static_cast<int>` only after the `std::in_range<int>` check documents and proves that the conversion is safe.
- Review other external-data narrowing in the two repositories, but change only boundaries where the source type can actually exceed the destination. Do not mechanically replace numeric casts used for arithmetic widening, SDK interop, or already-proven array bounds.

Tests:

- `#2147483647` parses as an int-valued slot and resolves according to normal no-match behavior.
- `#2147483648`, `#9223372036854775807`, and negative slot tokens do not produce a narrowed slot.
- Int command arguments accept INT_MIN and INT_MAX.
- Values immediately outside the int range return the existing bad-number error.

Acceptance:

- No parsed int64_t reaches an int slot or command argument through an out-of-range narrowing conversion.

## Phase 2: make CI execute the test suite

**Done 2026-08-26.** Baseline recorded: plugins `d224df4`, voltmod `0a942fe`, both on
`refactor/api-v2`, both worktrees otherwise clean; 294 framework + 158 plugin tests pass,
lint and modgraph green in both repos.

Changes: `ci.yml` now builds the framework from the source checkout and runs
`voltmod test linux-steamrt-release` before the package build, and the job is renamed to
"Build and test". `voltmod build` gained `--no-lockfile` (`buildtools.build(use_lockfile=)`)
because CI resolves against SDK packages it just built from HEAD recipes that `conan.lock`
does not pin - the same reason `package build kit` already passes it.

Also found and fixed a hole the phase text did not anticipate: `ctest` exited **0** when a
filter matched no cases, so a suite that silently stopped registering tests would have
reported success. All three `test-base` presets (voltmod, plugins, `templates/project`) now
set `"execution": { "noTestsAction": "error" }`. Verified both directions locally - an empty
run exits 1, a full run exits 0 with 294 cases. `docs/testing.md` gained a CI section.

Not done here: the acceptance line about proving a *failing* doctest reddens the workflow
needs a push to GitHub, so it is checked on the branch's next CI run rather than locally.

Problem:

- vendor/voltmod/.github/workflows/ci.yml builds packages but never runs CTest.

Primary file:

- vendor/voltmod/.github/workflows/ci.yml

Implementation:

- After SDK packages are available, configure the framework source checkout with testing enabled and run the Linux Steam Runtime test preset.
- Prefer the repository command used locally, such as uv run poe test with the explicit Linux preset if required by the CLI.
- Keep the Conan package build because it verifies package creation; do not assume it executes the source test suite.
- Make test failure fail the job.
- Avoid running the same expensive framework compilation twice if the CLI can share the configured build directory.

Acceptance:

- CI logs show discovered CTest cases and a pass count.
- A deliberately failing doctest fails the workflow during local workflow validation or a temporary test commit.

## Phase 3: remove confirmed unused surface

**Done 2026-08-26.** Six commits, ~870 lines removed from the framework. Framework 281 tests
green (294 -> 281 as tests for deleted surface went), plugins 158 green, lint and modgraph
clean in both.

3.1 removed Hooks::Damage (with Capability::Damage, the OnTakeDamageAlive binding, the
CCSPlayerPawn vtable ref, the CTakeDamageInfo/hitbox offsets and 7 gamedata entries), the
InputHistory service, Entities::EffectOps and Movement::PostCmd. HitGroup stayed - PlayerHurt
carries it and anticheat reads it. The Runtime canary went too, by request.

3.2 removed Args::Targets, SetMode::Server, MetamodPlugin::Rt/IsLateLoad, three Json helpers,
Strings::Split, the caller-id CallbackRegistry::Add overload, four Field operators, and the
plugin-side MayUseOn. Two things were *kept* against the plan's list because reference search
disproved it: Field `|=`/`&=`/`==` are used (PawnOps, FunMode, Hide), and JsonConfig::Mutable
is used (admin-system Config.cpp:27). TargetRules::AllowMultiple also stayed - it is internal
to src/ and is what lets the selector-grammar tests cover multi-match filtering.

3.3 removed the manifest/identity layer after the investigation below.

**ServiceExchange investigation (the prerequisite you asked for).** The reported
"dependency 'admin-system' is not loaded" warning could not be reproduced statically: the
manifests generate and install correctly, admin-system publishes `IdentityKey("admin-system")`,
anticheat looks up the identical string, and both go through the same `MetaFactory` path. The
note predates the api-v2 refactor and may be stale. Crucially the two paths are **not**
independent as first thought - `Exchange.Get<IAdminActions>()` and the identity lookup both call
`ServiceExchange::Query`, so deleting the manifest layer removed a duplicate symptom rather than
a unique signal. If cross-plugin lookup is ever broken, it now surfaces at the point of use,
where `ResponseManager` logs "cannot ban {}: admin-system is not loaded".

Residual: stale `*.manifest.json` files remain in the local CS2 server's addons/ from earlier
installs. Harmless (nothing reads them now); delete them if tidiness matters.

### Original scope

Perform this as several mechanical commits. For every deletion, remove its implementation, header exposure, build registration, capability, gamedata, tests that only test the deleted feature, and documentation.

### 3.1 Remove unused runtime services

Candidates confirmed by the current repositories:

- Hooks::Damage and DamageView.
- The InputHistory history-buffer service. Preserve UserCmdView::InputHistorySamples and related anticheat data.
- Entities::EffectOps.
- Movement::PostCmd.

For Damage, also remove:

- Capability::Damage.
- OnTakeDamageAlive and TakeDamage field bindings.
- The corresponding gamedata entries and schema descriptions.
- Hook API umbrella exposure and hook documentation.
- Stale admin-system comments that refer to the unused service.

For InputHistory service, also remove:

- Its construction from HookServices.
- Enable/depth API and per-slot buffers.
- Service documentation.
- Do not remove decoded user-command history or its tests.

Acceptance:

- No reference to the deleted service names remains outside historical documentation.
- Framework and plugin tests still pass.

### 3.2 Remove unused command, convar, field, and utility surface

Candidates:

- Args::Targets and its ArgKind, BoundArg variant entry, router branch, traits, tests, and docs.
- SetMode::Server and its unreachable write path.
- Unused Field operators: only `+=`, `<=>`, value-copy Field assignment, and explicit `operator bool`.
  Reference search confirms `|=` and `&=` ARE used (src/Entities/PawnOps.cpp:69,71) and `==` IS used
  (plugins/admin-system/src/Fun/FunMode.cpp:128, Admin/Effects/Hide.cpp:61). Do not delete those.
- ~~JsonConfig::Mutable~~ - USED at plugins/admin-system/src/Core/Config.cpp:27. Keep.
- MetamodPlugin::Rt and IsLateLoad.
- Unused Json Serialize, Deserialize, and SerializeToFile helpers.
- Strings::Split if only its own tests use it.
- Public Strings::ReplaceAll; make it a source-local helper used by token substitution.
- CallbackRegistry overloads with no production caller.
- Unused Controller, Pawn, EntitySystem, and wrapper convenience members identified by a fresh symbol-reference pass.

Rules:

- Do not delete a primitive merely because only a meaningful behavioral test calls it; first confirm the production feature using it is also absent.
- Keep small primitives that enforce an active safety invariant.
- Update CLAUDE.md and docs when a documented API disappears.

### 3.3 Remove manifest dependency reporting, separately

This is a behavior decision, not purely dead code.

Current behavior:

- Generated plugin manifests publish identity and version.
- Anticheat declares an admin-system dependency.
- Missing or old dependencies produce a delayed warning.
- Runtime use already null-checks the IAdminActions exchange contract.

Proposed change:

- Remove PluginManifest parsing, PluginIdentity, IPluginIdentity, identity keys, named identity publication, manifest load stage, dependency declarations, and their tests.
- Keep typed ServiceExchange contracts and their versioned InterfaceName.
- Keep ordinary plugin metadata and VDF generation.
- Verify deploy tooling does not consume the JSON manifest before deleting CMake generation.

Acceptance:

- Anticheat loads without admin-system and degrades through the existing null contract.
- Anticheat loads with admin-system and retrieves IAdminActions.
- Plugin packaging and installation no longer expect the deleted manifest.

If the startup version warning is operationally valuable, retain this subsystem and remove it from the cleanup scope.

## Phase 4: reduce public include and dependency cost

### 4.1 Remove magic_enum from the public API

Current use spans approximately these enum domains:

- Capability.
- StageStatus.
- ClientCvarStatus.
- GameData::Kind.
- ArgKind.
- Any additional use found by a final Name, Parse, EnumCount, EnumIndex, and EnumValues search.

Implementation:

- Add explicit Count sentinels only where indexed storage or iteration needs one.
- Declare narrow Name(Enum) overloads beside the enum.
- Define them in source files with switch statements or constexpr arrays.
- Delete unused generic Parse, EnumIndex, and EnumValues features rather than recreating them.
- Remove Core/EnumNames.hpp from VoltMod/Api.hpp.
- Remove magic_enum from Conan requirements and linked targets.
- Update CLAUDE.md, which currently mandates EnumNames.hpp.

Acceptance:

- No magic_enum include or Conan dependency remains.
- Unknown enum values return a stable fallback string or empty string, as decided per existing behavior.
- Capability indexing remains bounds-safe.

### 4.2 Narrow VoltMod/Api.hpp

Remove root-umbrella includes whose consumers already use specific module headers:

- Core/EffectManager.hpp.
- Players/ActionDispatcher.hpp.
- Players/EffectDescriptor.hpp.
- Core/Validation.hpp.
- Messaging/ChatColors.hpp.

KEEP Players/EffectDispatcher.hpp in the umbrella: it transitively supplies EffectManager,
ActionDispatcher and EffectDescriptor, and removing it breaks 8 translation units. Keeping
that one line makes the other five removals safe.

Migration:

- Add direct includes at each actual plugin or framework use.
- Keep Runtime, plugin lifecycle, commands, players, Result, Event, Subscription, and other genuinely ubiquitous vocabulary.
- Preserve the rule that root Api.hpp does not expose Menu/Flow or nlohmann.

Verification:

- Keep every tests/Api surface translation unit.
- Build with clean PCH/cache at least once so accidental transitive includes are exposed.

### 4.3 Keep StatusService lightweight without adding a new abstraction

Decision:

- Do not restore nlohmann::json to StatusService.hpp.
- Do not introduce a recursive framework-owned JSON variant solely for status.
- Status commands are infrequent, so parsing provider text on demand is acceptable.

Cleanup:

- Correct the false comment that says provider text is spliced without parsing.
- Rename Provider to JsonProvider if that improves clarity.
- Detect discarded parse results and emit a deterministic invalid-section value or error instead of silently carrying discarded JSON.
- Add one malformed-provider test.
- Keep App/Config.hpp if it remains the intentional JSON opt-in umbrella.

## Phase 5: simplify boilerplate and apply targeted C++23 cleanup

### 5.1 Admin effect tables

- Replace EffectEntry, which wraps one pointer, with an array of const EffectDescriptor pointers or references.
- Remove impossible null branches when construction guarantees non-null entries.
- Preserve explicit menu ordering.

### 5.2 FunMode convar handles

- Replace ToggleHandle with std::variant<ConVar<float>, ConVar<bool>>, or split the one bool row from the numeric rows.
- Resolve handles once during initialization.
- Use one std::visit or two short loops in ApplyOverrides.
- Do not bypass type checking by treating the bool convar as float.

### 5.3 Menu authorization helpers

- Delete unused MayUseOn.
- Keep one helper for single-admin menus if it improves readability.
- Keep the specialized punishment eligibility check where it also checks punishment type/state.
- Do not migrate everything to builder.Allowed under the false assumption that it is dynamically evaluated.
- After Phase 1, use PlayerRef for all delayed menu actions.

### 5.4 Default trailing reasons

- Add a generic builder facility for an optional trailing argument with a default value or default-producing callback.
- Keep translation lookup timing explicit: server/default language versus caller language must not become accidental.
- Replace ReasonOr and duplicated inline fallback expressions.
- Do not introduce a punishment-specific framework type unless other commands need the same semantics.

### 5.5 Descriptor ownership

- Keep ActionContext narrow.
- Keep descriptors that capture load-cycle services owned by App.
- After relaxing the forward-declaration rule, replace Core/Types.hpp with local App forward declarations where legal.
- Capture the narrowest needed service in each MakeX factory where practical.
- Do not convert descriptors back to static objects if they would capture per-load state.

### 5.6 Command registration ownership

- Keep returned command Subscriptions owned by App.
- Keep the existing Subs& parameter unless a replacement is demonstrably simpler at call sites.
- Do not introduce a Registrar class or vectors returned from seven one-caller functions merely to save indentation.
- Apply formatting separately from semantic changes.

### 5.7 Mechanical source cleanup

- Remove the duplicate MenuBuilder using declaration.
- Make file-local constants consistently static constexpr.
- Consolidate using declarations into one block inside the plugin namespace.
- Remove UTF-8 BOMs from the 19 affected sources (11 under plugins/, 8 under vendor/voltmod/,
  including include/VoltMod/Hooks/{Damage,Movement,Teleport}.hpp and Entities/Pawns.hpp) through the normal formatter or an encoding-preserving mechanical rewrite.

### 5.8 Standardize whole-container algorithms on ranges

Scope:

- Apply this cleanup in both vendor/voltmod and the parent plugins after the correctness phases are green.
- Prefer range overloads when an algorithm consumes an entire container.
- Use projections when a comparator or predicate exists only to select one data member.

Primary candidates:

- vendor/voltmod/src/Players/PlayerManager.cpp
- vendor/voltmod/src/Commands/CommandRouter.cpp
- vendor/voltmod/src/Database/Migrator.cpp
- vendor/voltmod/src/Hooks/ClientCvarPending.cpp
- vendor/voltmod/src/Engine/ConVarSnapshots.cpp
- vendor/voltmod/src/Engine/Precache.cpp
- plugins/admin-system/src/Admin/AdminManager.cpp
- plugins/admin-system/src/Admin/Menu/AdminMenu_ChatSettings.cpp
- plugins/admin-system/src/Punishments/PunishmentManager.hpp
- plugins/anticheat/src/Core/WeaponClass.cpp
- plugins/anticheat/src/Response/ResponseManager.cpp

Implementation:

- Replace whole-container `std::sort`, `std::find`, `std::find_if`, `std::count_if`, `std::any_of`, `std::all_of`, and `std::transform` calls with their `std::ranges` equivalents where the result and traversal order remain identical.
- Prefer projections such as `std::ranges::sort(migrations, {}, &Migration::Version)` and `std::ranges::find(entries, value, &Entry::Member)` over one-line member-selection lambdas.
- Use `std::ranges::contains` for vector or array membership checks instead of comparing a find result with end.
- Use `std::views::keys` or `std::views::values` only for map loops that genuinely ignore the other half of a structured binding.
- Keep reverse-iterator anticheat searches unchanged: newest-first traversal is a useful fast path and some callers need the reverse iterator itself.
- Confirm `__cpp_lib_ranges_contains >= 202207L` on both the configured MSVC and GCC 14 builds before adopting contains as a convention. If either configured standard library lacks it, keep `std::ranges::find`; do not add a compatibility shim.

Acceptance:

- Algorithm migrations reduce iterator-pair and comparator boilerplate without changing allocation, traversal, mutation, or error behavior.
- Both configured toolchains compile the same source without conditional compatibility code.

### 5.9 Replace immutable runtime containers with compile-time tables

Primary candidates:

- plugins/admin-system/src/Admin/Effects/Model.hpp
- plugins/admin-system/src/Admin/Effects/Model.cpp
- plugins/admin-system/src/Admin/Menu/AdminMenu_ChatSettings.cpp
- plugins/admin-system/src/Fun/FunToggles.hpp
- plugins/anticheat/src/Core/Finding.hpp
- plugins/anticheat/src/Config.hpp

Implementation:

- Change FunModel fields to `std::string_view` and replace the function-local static vector with an inline constexpr `std::array`.
- Remove the FunModels accessor function if direct access to the constexpr table is clearer at every call site.
- Replace the fixed color-label `std::unordered_map` with a constexpr array searched through a ranges projection. This table is small, read-only, and only used while building a menu; it does not need hashing or dynamic initialization.
- Prefer `std::array` over raw fixed-size arrays for toggle and detection tables when it improves size, iteration, and indexing contracts.
- Use `std::to_underlying` for enum-to-underlying conversions in effect ids, permissions, toggle indices, and detection indices.
- Keep explicit casts for integer-to-enum conversion, SDK ABI conversion, and conversions whose destination deliberately differs from the enum's underlying type.

Acceptance:

- Literal lookup data performs no heap allocation or dynamic container initialization.
- Enum indexing states the underlying-type intent without hiding unrelated numeric conversions.

### 5.10 Narrow string inputs and remove avoidable allocations

Primary files:

- vendor/voltmod/include/VoltMod/Core/Strings.hpp
- vendor/voltmod/src/Core/Strings.cpp
- vendor/voltmod/src/Commands/CommandRouter.cpp
- vendor/voltmod/src/Commands/Targeting.cpp
- vendor/voltmod/tests/Core/StringsTests.cpp

Implementation:

- Change read-only string utility inputs such as ToLower, Trim, StartsWith, ContainsIgnoreCase, and IsNumeric to `std::string_view` where lifetime is not retained.
- Remove explicit `std::string` temporaries created only to satisfy the old utility signatures.
- Replace the StartsWith wrapper with `std::string_view::starts_with` at its few call sites if deleting the wrapper leaves the code clearer.
- Evaluate `std::ranges::contains_subrange` with a case-insensitive comparison for ContainsIgnoreCase so it no longer lowercases and allocates two complete strings.
- Preserve the existing empty-needle behavior and unsigned-char conversion before calling ctype functions.
- Do not turn these utilities into generic range templates; the intended domain is textual data, and templates would increase public header and diagnostic cost.

Tests:

- Preserve current empty-string, ASCII case-folding, whitespace, and numeric behavior.
- Add coverage for string_view substrings that are not null terminated.
- Confirm CommandRouter Find and Remove no longer allocate an input copy before lowercasing the key.

Acceptance:

- Read-only string APIs accept views and do not force caller-side ownership allocations.
- Case-insensitive containment does not allocate lowercase copies.

### 5.11 Strengthen ignored-result contracts selectively

Implementation:

- Add `[[nodiscard("handle the error or cast the result to void")]]` to public Status/Result-returning operations where ignoring an engine failure is usually a bug, including ConVar Set/SetFor and entity mutations such as Slay, Kick, ChangeTeam, Respawn, and Teleport.
- Update intentional fire-and-forget call sites with an explicit `(void)` cast; handle errors where the caller can provide useful operator or player feedback.
- Do not wrap `std::expected` in a project-specific class merely to apply a type-level attribute.
- Do not mark fluent builders, status probes, or APIs whose result is routinely and safely optional without reviewing their call sites first.

Acceptance:

- Accidental loss of an actionable engine error produces a compiler diagnostic.
- Intentional discards are visible in source and do not create warning noise.

### 5.12 Modernization guardrails

- Do not rewrite FilterRoster's staged candidate mutation as a filter/transform pipeline; its intermediate empty checks preserve the reason targeting failed.
- Do not use `views::enumerate` or `views::iota` merely to replace the short indexed model-choice loop unless the constexpr-table change leaves a demonstrably clearer expression.
- Do not use `ranges::to`, `fold_left`, or expected/optional monadic chains for code that is clearer as a reserved vector plus loop or an explicit early return.
- Do not convert Event or Scheduler callbacks wholesale to `std::move_only_function`. CallbackRegistry deliberately copies an item before invocation so self-unsubscription cannot destroy the running callback, and Scheduler depends on that dispatch behavior.
- Keep `std::move_only_function` in database completion ownership, where move-only captures are useful and the callback is not copied for re-entrant dispatch.
- Do not add `std::scope_exit` to Migrator merely to replace the explicit advisory unlock. The normal path unlocks explicitly; an exception makes the database worker drop the PostgreSQL connection, which releases the session lock.
- Do not introduce `std::generator`, `std::print`, `std::flat_map`, `std::unreachable`, or deducing-this patterns without a concrete consumer and a measured reduction in code or risk.

## Phase 6: simplify tests and test seams carefully

### PerSlot

- Merge PerSlotTests.cpp and PerSlotBoundsTests.cpp if one file is easier to navigate.
- Keep both behavior groups:
  - reset binding and Subscription lifetime.
  - invalid-slot bounds safety.
- Remove only duplicated setup and includes.

### Event and CallbackRegistry

- Keep Event tests for:
  - normal multicast behavior.
  - Subscription drop.
  - lifecycle first/last behavior.
  - refused lifecycle and retry.
  - at least one reentrant unsubscribe case.
- Keep CallbackRegistry tests for its snapshot and mutation algorithm.
- Remove duplicate Subscription move tests if Subscription already has direct coverage.
- Do not assume one-line delegation makes integration behavior unworthy of a test.

### Test-only seams

Review:

- ArgBinder.
- VtableHook injected add/remove callbacks.
- SetFieldQuery and ResetFieldCache.
- MenuBuilder(Policy&, title).

For each:

- Keep the seam if it is also a clean architectural boundary, such as separating CommandRouter from engine target resolution.
- Otherwise move it to a Detail/internal header, a friend test adapter, or a compile-time testing path.
- Preserve tests for the behavior the seam makes observable.
- Avoid production preprocessor branches unless the alternative is substantially worse.

### Result tests

- Keep Result and ErrorCode for this pass.
- Delete tests that merely prove a trivial factory initializer only if stronger caller-level tests cover the same code.
- Revisit taxonomy only after the correctness phases reveal whether callers should branch on ErrorCode.

## Phase 7: simplify modgraph and repository rules

Primary files:

- vendor/voltmod/scripts/voltmod/modgraph.py
- vendor/voltmod/tests or Python tests for modgraph, if present
- CLAUDE.md
- vendor/voltmod/CLAUDE.md
- vendor/voltmod/docs/architecture.md
- vendor/voltmod/docs/testing.md

Keep:

- ALLOWED module dependency DAG.
- Root Runtime/Api include restrictions.
- The nlohmann direct-include boundary.
- Core versus engine/SDK include checks when they provide a clearer failure than the build.

Remove or relax:

- The global anonymous-namespace ban.
- The global forward-declaration ban and forced Types.hpp convention.
- The using-directive ban in source files. Continue banning using-directives in headers.
- LOWER_MODULES/CROSS_MODULE_HEADER checks if ALLOWED reports the same edge clearly.
- Stale nlohmann allowlist entries.
- Duplicated reporting code that does not add a distinct invariant.

Acceptance:

- A deliberately forbidden module edge still fails.
- A root Runtime include from a lower module still fails.
- A direct nlohmann include outside the allowlist still fails.
- An anonymous namespace or reasonable forward declaration in a source/header succeeds under the revised policy.
- Parent plugin lint uses the revised rules successfully.

## Phase 8: documentation consolidation

Do this after APIs stabilize so documentation is rewritten once.

Canonical ownership:

- mainpage.md: module overview and navigation.
- architecture.md: design reasons, dependency direction, lifecycle, and cross-module contracts.
- plugin.md: plugin load/unload composition and registration.
- players.md: roster, PlayerRef, Policy, actions, and effects.
- sdk/entities.md: entity wrappers and Field behavior.
- sdk/gamedata.md: authoring, validation, drift diagnosis, and binding safety.
- sdk/hooks.md: hook-specific APIs and VtableHook rules.
- testing.md: how to run and add tests.
- consuming-via-conan.md: package consumption and the coordinated development workflow.

Specific consolidation:

- Keep one module list; link to it elsewhere.
- Keep one complete Policy installation example; use short links elsewhere.
- Keep one complete Subscription ownership explanation.
- Keep one typed game-event list.
- Keep Field mechanics in sdk/entities.md; link from gamedata.
- Keep the hook lifecycle contract once, then state only hook-specific differences.
- Remove the constructor-by-constructor Runtime inventory from architecture.md.
- Replace the copied ALLOWED table with a short explanation and a link to modgraph.py, or generate it if the rendered table is important.
- Retain the one-vfunc-per-translation-unit rule because shared descriptor mutation is non-obvious; shorten it without reducing it to only the duplicate-symbol symptom.
- ServerCommand may remain in sdk/hooks.md because that guide explicitly covers server commands; rename or split the guide only if navigation improves.
- Keep internal signature/vtable resolver details that are necessary for maintaining gamedata. Move them to a maintainer subsection rather than deleting them solely because plugin code cannot call them.
- Reduce the Runtime canary section to a short diagnostic note. Keep the canary until the original overwrite cause is proven eliminated and runtime smoke testing has soaked the new layout.

Coordinated development workflow:

- Remove duplicated copies of the same package/lock commands across CLAUDE.md, CONTRIBUTING.md, local-development.md, and consuming-via-conan.md.
- Choose one canonical guide and link to it.
- Investigate restoring a Conan editable workflow for rapid framework/plugin iteration.
- Keep package creation plus lockfile refresh as the reproducibility and pre-merge gate.
- Do not document an editable command until it has been exercised against the current Conan recipe and CMake build modules.

## Phase 9: integration and verification

### Framework verification

Run from vendor/voltmod:

- uv run poe format
- uv run poe lint
- uv run poe modgraph
- uv run poe test
- uv run poe build windows-msvc-debug
- uv run poe build-linux, where the Steam Runtime environment is available
- git diff --check
- git status --short

Check:

- No new warnings beyond understood SDK/toolchain warnings.
- All API-surface compile targets build.
- CTest count is non-zero in CI and locally.

### Package and parent integration

After coherent framework commits:

- Build the local VoltMod package once.
- Refresh the parent conan.lock once with the repository's canonical profile and options.
- Confirm the parent resolves the intended VoltMod revision.
- Commit the parent lockfile separately from framework commits.

### Plugin verification

Run from the parent repository:

- uv run poe format
- uv run poe lint
- uv run poe test
- uv run poe build windows-msvc-debug
- uv run poe build-linux, where available
- git diff --check
- git status --short

### Runtime smoke tests

Use a local CS2 server for behaviors unit tests cannot cover:

1. Load all plugins normally.
2. Meta reload each plugin.
3. Force admin-system OnLoad failure and confirm no SetClientListening callback survives.
4. Open an admin target menu, disconnect the target, reuse the slot, and confirm the old row is inert.
5. Revoke an admin permission while a menu is open and confirm activation is denied.
6. Load malformed gamedata and confirm a controlled degraded/failure report rather than an exception.
7. Use a deliberately ambiguous signature in a test gamedata copy and confirm the dependent capability is off.
8. Exercise anticheat without admin-system and confirm its exchange null path remains safe.

## Suggested commit sequence

Keep commits narrowly reversible.

Framework repository:

1. test(ci): run VoltMod CTest suite in CI
2. fix(gamedata): reject ambiguous and structurally invalid entries
3. fix(app): own custom hooks through shutdown
4. fix(players): preserve PlayerRef through action and menu dispatch
5. fix(convars): reject narrowing typed handles
6. fix(commands): restrict PlayerOrSteamId fallback
7. fix(commands): reject out-of-range numeric narrowing
8. chore(runtime): remove unused hook and entity services
9. chore(api): remove unused public helpers and argument kinds
10. refactor(api): narrow root umbrella and remove magic_enum
11. refactor(core): use targeted ranges, views, and string_view contracts
12. refactor(tooling): reduce modgraph to architectural checks
13. docs: consolidate API v2 guidance

Parent plugin repository:

1. fix(admin): register custom hook with base ownership
2. refactor(admin): migrate menu dispatch to PlayerRef
3. refactor(admin): simplify effect tables and convar variants
4. refactor(commands): use generic default trailing arguments
5. refactor(core): use constexpr tables, ranges projections, and to_underlying
6. chore(source): normalize using blocks, constants, and UTF-8 encoding
7. build: refresh VoltMod package lock
8. docs: point local development guidance at the canonical workflow

The exact split may change when a framework API and its only consumer must compile together. Even then, keep the changes as paired commits in the two repositories rather than one mixed worktree commit.

## Definition of done

- All Phase 1 correctness regressions have tests or documented runtime smoke coverage.
- VoltMod CI runs CTest.
- Confirmed zero-use services and members are removed.
- No detached scheduling or ambient Runtime access was introduced.
- Targeted C++23 changes compile on both configured toolchains without compatibility shims.
- Range and view migrations reduce boilerplate without obscuring staged error handling or callback lifetime behavior.
- Root Api.hpp no longer exposes admin-only facilities or magic_enum.
- modgraph enforces architecture rather than broad lexical style.
- Documentation has one canonical home per concept.
- Framework and plugin test/lint/build gates pass.
- Both worktrees are clean after their independent commits.
- The parent lockfile resolves the intended final VoltMod package revision.
