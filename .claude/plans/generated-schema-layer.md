# Build-level generated schema for VoltMod

Status: planned 2026-08-29, not started. Implement phase by phase; each phase is its own commit pair (voltmod, then consumer with the relocked `conan.lock`).

## 1. Why and the decision

Schema field access is the main source of low-level debug pain: fields are hand-declared one at a time, resolution is lazy (a renamed field surfaces mid-round as one `Log::Warn` plus silent zero reads), the size check can't catch type changes, and sub-object writes need hand-rolled replication workarounds.

Decision (after reviewing plugify-s2sdk and SwiftlyS2 in `references/` and two design reviews): **generate the schema layer at build level from a schema dump, with offsets baked in, and delete the runtime schema resolver entirely.** One source of truth (`schema/server.json` + a manifest), one generator, plain C++ output. The cost is explicit: a CS2 update that shifts a used class's layout requires dump → regenerate → rebuild → redeploy. That is accepted; in exchange the framework loses the resolver, cache, pending-retry logic, `FieldRef`/`SchemaField`/`Field`/`SchemaPtr`, `Capability::Schema`, and the contract-validation machinery a runtime design needs.

Non-negotiable safety net: **a one-shot check at load that the live schema matches the generated offsets and sizes, refusing to load on mismatch.** A stale baked offset with no check is silent memory corruption; the check is one small SDK file with no caching or resolution, not a resolver.

Principles:

- Do not over-engineer. Generated headers are plain C++ (accessor methods with direct types, no templates, no string literals). Plumbing lives in generated `.cpp` files nobody maintains.
- Names in the manifest, offsets from the dump; the generator never guesses.
- Generated mutation exists only where the dirty route is known: entity classes notify directly, component classes notify through their `__m_pChainEntity` chainer, everything else is read-only.
- Generated types keep schema names verbatim in `VoltMod::Schema::` (a justified nested namespace, same rationale as `VoltMod::Args`: names identical to the SDK's globals cannot sit at `VoltMod::` scope).

## 2. Ground truth

- Current runtime schema layer to remove: `vendor/voltmod/include/VoltMod/Entities/Field.hpp` (`FieldRef`, `FieldKey`, `ResolveField`, `PendingField`, `SchemaField<T>`, `Field<T, Klass, Name, ExpectedSize>`; **`CharBuf<N>` and `FixedString` stay** — they are value types), `SchemaPtr.hpp`, `src/Entities/SchemaResolve.{hpp,cpp}` (cache, seam), `src/Entities/SchemaQuery.cpp` (SDK backend, `MarkChanged`, `NetworkVarChainer`), `Capability::Schema` (`Core/Capabilities.hpp:15`, set at `src/Runtime.cpp:160` via `BindSchemaSystem`), tests `tests/Entities/FieldRefTests.cpp`, `tests/Entities/SchemaPtrTests.cpp`, and the `SchemaResolve.cpp` entry in the `voltmod-utils-tests` source list (root `CMakeLists.txt` ≈:145).
- `MarkChanged` semantics to preserve (SchemaQuery.cpp ≈:124-144): a field on a class with `__m_pChainEntity` notifies `chainer->Entity->NetworkStateChanged(NetworkStateChangedData(offset, -1, chainer->PathIndex))` where the chainer (`{CEntityInstance* Entity; uint8_t Pad[24]; ChangeAccessorFieldPathIndex_t PathIndex}`, `static_assert(offsetof PathIndex == 0x20)`) sits at `base + chainOffset`; otherwise `entity->NetworkStateChanged(NetworkStateChangedData(offset))`. Per commit `85a526d` writes always notify (the shipped binary has no `MNetworkEnable` metadata, so "networked" is unknowable and irrelevant).
- Consumers of the runtime layer (all migrate to generated types): wrappers `include/VoltMod/Entities/Entity.hpp:93-101`, `Pawn.hpp:30-51`, `Controller.hpp:36`; file-scope statics in `src/Entities/Entity.cpp` (:18-178), `EntitySystem.cpp:154-180`, `Items.cpp:13`, `Render.cpp:10-11`, `src/Hooks/Transmit.cpp:25-29`, `src/Hooks/Vote.cpp:34-39`, `src/Ui/UiFields.cpp:17-22,194-208`; stored `SchemaPtr`s in `Hooks/Vote.hpp:110`, `Hooks/Movement.hpp:64,73,87`, `Entities/EntitySystem.hpp:97`, `Entities/Items.hpp:50`; one plugin site `plugins/ui-lab/src/Commands.cpp:28,83`. Total: **20 classes / 52 fields** (inventory in section 7).
- Load flow: `src/App/MetamodPlugin.cpp` `Load` (≈:45): `Runtime::Start` → `RegisterStandardHooks` → `OnRegisterHooks` → `OnLoad` → "Commands" stage → summary. `Runtime::InitializeServices` (`src/Runtime.cpp:108-205`) records `LoadReport` stages; only "Messages" is load-aborting today (`StageResult::Failed`). `runtime.Unsafe.Interfaces.SchemaSystem` (`Engine/Interfaces.hpp:31`) is public.
- SDK headers (conan package `C:\Users\admin\.conan2\p\b\hl2sd4502d2d9978ad\p\public\schemasystem\`): `CSchemaSystemTypeScope` public `m_ClassBindings`/`m_EnumBindings` (`CUtlTSHash`: `Count()`, `GetElements()`, `Element()`); `ISchemaSystem::SchemaSystemIsReady()`, `FindTypeScopeForModule`, `FindDeclaredClass`; `SchemaClassInfoData_t` (`m_pszName`, `m_nSize`, `m_nAlignment`, `m_nFieldCount`, `m_pFields`, `m_nBaseClassCount`, `m_pBaseClasses[i].{m_nOffset,m_pClass}`, flags incl. `SCHEMA_CF1_IS_ABSTRACT`); `SchemaClassFieldData_t` (`m_pszName`, `m_pType`, `m_nSingleInheritanceOffset`); `CSchemaType` (`m_sTypeName`, `m_eTypeCategory`, `GetInnerType`, `GetSizeAndAlignment`, `CSchemaType_FixedArray::m_nElementCount`, `CSchemaType_DeclaredClass::m_pClassInfo`, `CSchemaType_DeclaredEnum::m_pEnumInfo`); `SchemaEnumInfoData_t`/`SchemaEnumeratorInfoData_t`.
- CLI: `vendor/voltmod/scripts/voltmod/cli.py` (Typer; sub-Typer precedent `builder/package.py`, `app.add_typer(...)` at :22); artifact producers live in `builder/`; pytest in `scripts/tests/` (`pythonpath=["scripts"]`, run by `poe test-tools`, part of `poe lint`); codegen precedent `scaffold/new_plugin.py` (`write_text(..., newline="\n")`). modgraph allowlist `scripts/voltmod/checks/modgraph.py`; `scripts/tests/test_modgraph.py:124-128` asserts the fenced layering blocks in voltmod `CLAUDE.md` and `docs/architecture.md` match.
- Build: `src/<Module>/*.cpp` are globbed with `CONFIGURE_DEPENDS` (`CMakeLists.txt:30-45`); `gamedata/` installs wholesale into servers (`:92`). New committed schema inputs go in a top-level `vendor/voltmod/schema/` that no `install(DIRECTORY ...)` covers.
- Tests: doctest, SDK-free only; case names must not contain `[`, `]`, `;`.
- A per-plugin JSON manifest layer was deliberately removed before (`.claude/plans/refactor-hardening-and-simplification.md:545-638`); `schema/manifest.json` is a framework build input, not a plugin manifest.

## 3. Phase 1 — schema dumper as a dedicated dev plugin

- New consumer-repo plugin `plugins/schema-dump/` (`uv run poe new-plugin schema-dump`; dev-only like `plugins/ui-lab`, never deployed). Registers `schema_dump [path]` through the normal command builder; reaches the schema system through `runtime.Unsafe.Interfaces.SchemaSystem` plus `<schemasystem/schemasystem.h>` / `<schemasystem/schematypes.h>` directly. Nothing enters `voltmod-runtime`; command ownership is unambiguous.
- Handler: if `!SchemaSystemIsReady()` reply "schema system not ready" and return. Default output: the plugin's addon directory + `schema/server.json`; argument overrides. Reply with path and class/enum counts.
- Walk `FindTypeScopeForModule(PlatformModuleName("server"))`: `m_ClassBindings` and `m_EnumBindings` via `Count()`/`GetElements()`/`Element()` (pin against the conan headers above; the API differs across SDK revisions).
- JSON IR, format version 1, compact, `\n`:

```json
{
  "format": 1,
  "scope": "server",
  "classes": {
    "CCSPlayerPawn": {
      "size": 4992,
      "bases": [{"name": "CCSPlayerPawnBase", "offset": 0}],
      "chain_offset": -1,
      "fields": [
        {"name": "m_ArmorValue", "offset": 5004, "size": 4, "type": {"name": "int32", "category": "builtin"}},
        {"name": "m_pItemServices", "offset": 2760, "size": 8,
         "type": {"name": "CPlayer_ItemServices*", "category": "pointer", "inner": "CPlayer_ItemServices"}},
        {"name": "m_szLastPlaceName", "offset": 3400, "size": 18,
         "type": {"name": "char[18]", "category": "fixed_array", "inner": "char", "extent": 18}}
      ]
    }
  },
  "enums": {
    "CSPlayerState": {"size": 4, "items": [{"name": "STATE_ACTIVE", "value": 0}]}
  }
}
```

  Type object: `name` (schema's own string), `category` (from `SchemaTypeCategory_t`), `inner` (pointee/element type), `extent` (fixed arrays), `declared` (`class`|`enum`). `chain_offset` is the class's own `__m_pChainEntity` offset walking single inheritance up (same walk as today's `FindChainOffset`), or -1. Only what the generator reads is in the file — no timestamp, alignment, abstract flag or project name — so the committed baseline only changes when the schema does.
- Verify live whether any type referenced by a server field lives outside the server scope; add the global scope only if one does.
- Verify: build + install, run `schema_dump` before a map loads (expect "not ready") and after; inspect `CCSPlayerPawn.m_ArmorValue` and a service class (`CCSPlayerController_InGameMoneyServices` has `chain_offset >= 0`). One README paragraph. Consumer-only commit.

## 4. Phase 2 — generator (`voltmod schemagen`) producing plain C++

### Output shape

One header per manifest class, `include/VoltMod/Schema/<Class>.hpp` (include granularity), plus `include/VoltMod/Schema/Enums.hpp` (all enums in the manifest closure) and **one** `src/Schema/Generated.cpp` holding every accessor definition and the layout table for the verifier (section 5). All generated and **committed** (regenerate, review the diff, commit — like `conan.lock`). New modgraph module `Schema -> {Core, Engine}`; `Entities`, `Hooks`, `Ui`, `App` gain `Schema` in their allowed sets (they will call into it). Update both fenced layering blocks (voltmod `CLAUDE.md`, `docs/architecture.md`).

Header — plain types, no templates, no strings:

```cpp
// Generated by `voltmod schemagen` from schema/server.json + schema/manifest.json. Do not edit.
#pragma once

#include <VoltMod/Engine/EngineTypes.hpp>
#include <VoltMod/Schema/CCSPlayerPawnBase.hpp>
#include <VoltMod/Schema/CPlayer_ItemServices.hpp>
#include <VoltMod/Schema/Enums.hpp>

namespace VoltMod::Schema
{

/** Frame-local view over an engine CCSPlayerPawn; never store one. Writes notify the engine. */
struct CCSPlayerPawn : CCSPlayerPawnBase
{
    using CCSPlayerPawnBase::CCSPlayerPawnBase;

    int32_t ArmorValue() const;
    void SetArmorValue(int32_t value) const;

    float VelocityModifier() const;
    void SetVelocityModifier(float value) const;

    CSPlayerState PlayerState() const;
    void SetPlayerState(CSPlayerState value) const;

    /** Null when the pawn has no item services. */
    CPlayer_ItemServices ItemServices() const;
};

}  // namespace VoltMod::Schema
```

Root of every entity chain is a generated `CEntityInstance` with `CEntityInstance* _e` and `explicit operator bool()`; non-entity roots hold `void* _base`. Multi-base schema classes (rare): first base is the C++ base, the others' fields are flattened in with a comment.

`Generated.cpp` — baked offsets, direct memory access through the existing `MemberPtr<T>` (`Engine/MemoryAccess.hpp`), notification through two tiny framework helpers (section 4, "runtime support"); one block per class:

```cpp
#include <VoltMod/Schema/CCSPlayerPawn.hpp>
#include <VoltMod/Schema/Notify.hpp>
// ... every other generated header

namespace VoltMod::Schema
{

// ---- CCSPlayerPawn, 4992 bytes ----
static constexpr int32_t kArmorValue = 5004;        // int32
static constexpr int32_t kVelocityModifier = 5016;  // float32
static constexpr int32_t kPlayerState = 5020;       // CSPlayerState
static constexpr int32_t kItemServices = 2760;      // CPlayer_ItemServices*

static_assert(sizeof(CSPlayerState) == 4, "CCSPlayerPawn::m_iPlayerState is 4 bytes in schema");

int32_t CCSPlayerPawn::ArmorValue() const { return _e ? *MemberPtr<int32_t>(_e, kArmorValue) : int32_t{}; }

void CCSPlayerPawn::SetArmorValue(int32_t value) const
{
    if (!_e)
        return;
    *MemberPtr<int32_t>(_e, kArmorValue) = value;
    NotifyEntity(_e, kArmorValue);
}

CPlayer_ItemServices CCSPlayerPawn::ItemServices() const
{
    return CPlayer_ItemServices{_e ? *MemberPtr<void*>(_e, kItemServices) : nullptr};
}

}  // namespace VoltMod::Schema
```

### Generated code style (deliberately plain)

Use:

- **Direct types in signatures** and `X()`/`SetX()` naming (matches `Pawn::Move()`/`SetMove()`); nothing templated, no string literals in headers.
- **`enum class Name : intN_t`** sized from the dump for every schema enum, so enum fields are read as their enum.
- **Named `static constexpr` offsets in the `.cpp`**, one per field with the schema type as a trailing comment — update-day diffs read `5004 -> 5012` on one line; the header never shows an offset.
- **`static_assert(sizeof(T) == <schema size>)`** in the `.cpp` for every mapped value type that is meant to cover the whole field; a `cpp.type` override that reads a leading value gets a `// leading value of <schema type>` comment instead of the assert.
- **`std::string_view`** for `char[N]` fields (getter returns a view into the entity; setter takes a view and truncates through `CharBuf<N>`).
- **Indexed getters for fixed scalar arrays** (`int32_t VoteOptionCount(size_t index) const`, bounds-checked against the extent), not spans.
- **Designated initializers** in the generated layout table (`{.Name = "m_ArmorValue", .Offset = 5004, .Size = 4}`) so the verifier's input is readable.
- **`explicit operator bool()`** as the only validity check, as on `Entity`/`Pawn`.

Skip: `[[nodiscard]]`/`noexcept` on accessors, `std::span`, `std::optional` for nullable components (a falsy view is the idiom), `std::bit_cast`, range adaptors, deducing `this`, concepts, modules, defaulted comparisons. Bitfield schema fields are skipped with a comment in v1.

Component classes with `chain_offset >= 0` get setters that call `NotifyThroughChain(_base, kChainOffset, offset)`; classes with no chainer and no entity root get **no setters** (read-only). Fixed `char[N]` fields return `std::string_view` and accept one via `CharBuf<N>`; fixed arrays of scalars get indexed getters; `CHandle<T>` fields return `uint32_t` (typed `EntityRef` later); pointer-to-declared-class returns the generated view; unmapped types are skipped with a `// skipped: <type>` comment so generation is total. `Enums.hpp` emits `enum class Name : <intN_t by size> { ... }` — `EnumNames` (magic_enum) works on them unchanged.

### Runtime support (hand-written, small, in the Schema module)

`include/VoltMod/Schema/Notify.hpp` + `src/Schema/Notify.cpp` (SDK-touching; replaces today's `MarkChanged`):

```cpp
/** Dirty a field on an entity for the next snapshot. */
void NotifyEntity(CEntityInstance* entity, int32_t offset);
/** Dirty a field on a component whose class embeds `__m_pChainEntity` at chainOffset. */
void NotifyThroughChain(void* component, int32_t chainOffset, int32_t offset);
```

The `NetworkVarChainer` struct and its `static_assert` move here from SchemaQuery.cpp. Nothing else is hand-written in the module.

### Manifest — `vendor/voltmod/schema/manifest.json` (committed)

Flat: class → field list (or `"*"`). The only override is a `:CppType` suffix on a field name, which reads the field as that C++ type (leading-value reads such as `CNetworkViewOffsetVector` → `Vector`, or a `CUtlVector` of handles as the existing `HandleVectorView`).

```json
{
  "CCSPlayerPawn": ["m_ArmorValue", "m_angEyeAngles", "m_flVelocityModifier", "m_bOnGroundLastTick"],
  "CBaseModelEntity": ["m_vecViewOffset:Vector", "m_nRenderMode", "m_clrRender"],
  "CVoteController": "*"
}
```

Seed with the 20 classes / 52 fields (section 7). A one-line header comment in the generated output (not the manifest) says it is a framework build input, not a plugin manifest.

### CLI — `scripts/voltmod/builder/schemagen.py`, sub-Typer wired in `cli.py`

- `voltmod schemagen --dump <full.json>` — the one command. Prunes the dump to the manifest closure (listed classes + transitive bases + declared inner types of listed fields + enums they use), writes the trimmed baseline to `schema/server.json` (committed), emits the headers, `Enums.hpp` and `Generated.cpp`. Accessor names: strip `m_`, then a leading lowercase Hungarian run followed by an uppercase letter (`m_flFlashDuration` → `FlashDuration`, `m_iAccount` → `Account`); collisions or all-lowercase leftovers are a hard error. Manifest field absent from the dump → error naming class/field. Writes `encoding="utf-8", newline="\n"`, pre-formatted to the pinned clang-format style so `poe format` is a no-op.
- There is no `diff` subcommand. With offsets baked into committed code, `git diff` after regenerating *is* the drift report (one `kArmorValue = 5004` → `5012` line per moved field), and the verification stage (section 5) is what says a regenerate is needed.
- Committing generated files rather than generating at build time is deliberate: `conan create` for the package must not depend on Python, and the diff of generated code is what gets reviewed on update day.

### Tests — `scripts/tests/test_schemagen.py` (pytest; under `poe lint`)

- Closure pruning includes bases, declared inner types and enums; excludes the rest.
- Setter emission: entity chain → `NotifyEntity`; `chain_offset >= 0` → `NotifyThroughChain`; neither → no setters.
- Name derivation and the collision error; the `:CppType` suffix override applied.
- Golden generation of one entity class, one component class, `Enums.hpp` and the `Generated.cpp` block (exact text, `\n`, trailing newline).
- Missing manifest field → error, nonzero exit. Regenerating from an unchanged dump produces no diff (no timestamp).
- Generated code compiles under normal `poe build`.

## 5. Phase 3 — startup layout verification (the safety net)

- The tail of `Generated.cpp` exposes `std::span<const ClassLayout> GeneratedLayout()` where `ClassLayout{std::string_view Name; int32_t Size; int32_t ChainOffset; std::span<const FieldLayout> Fields}` and `FieldLayout{std::string_view Name; int32_t Offset; int32_t Size}`, straight from `schema/server.json`.
- Hand-written `src/Schema/Verify.cpp` (SDK-touching, ~60 lines): `Status VerifySchemaLayout(ISchemaSystem&)` — for each class `FindDeclaredClass`, compare `m_nSize`, then for each field find it (own fields, then bases, most-derived first) and compare `m_nSingleInheritanceOffset` and type size; compare the chain offset. Collect every mismatch into one message grouped by class; no caching, no state.
- `Runtime::InitializeServices`: a `"Schema"` stage that **fails the load** (`StageResult::Failed`, like "Messages") with the aggregated message: `schema drift: CCSPlayerPawn::m_ArmorValue offset 5004 -> 5012 (regenerate with voltmod schemagen)`. Deferred only if `SchemaSystemIsReady()` is false at Metamod load — confirm live; if the scope is always ready then, no deferral code at all.
- Add a `"schema"` status section (verified class/field counts) beside "load"/"gamedata"/"capabilities"; remove `Capability::Schema` (its two uses in `plugins/anticheat` and `ui-lab` become unconditional or use `Capability::Entities`).
- No doctest (SDK-only); the live check is deliberately corrupting one offset in a scratch build and watching the load refuse with the right line.

## 6. Phase 4 — delete the runtime layer and migrate

Delete: `Field.hpp` contents except `CharBuf`/`FixedString` (move those to `Entities/CharBuf.hpp`), `SchemaPtr.hpp`, `SchemaResolve.{hpp,cpp}`, `SchemaQuery.cpp`, `FieldRefTests.cpp`, `SchemaPtrTests.cpp`, `Capability::Schema`, `BindSchemaSystem` and the Runtime.cpp:160 stage, the `SchemaResolve.cpp` line in the utils-tests source list, `docs/sdk/entities.md` sections on `Field`/`SchemaPtr`/`FieldRef`, and the `Field<T, "Class", "m_name">` bullets in both CLAUDE.md files.

**The curated wrappers inherit the generated types; they do not forward to them.** `Entity : Schema::CBaseEntity`, `Pawn : Schema::CCSPlayerPawn`, `Controller : Schema::CCSPlayerController` (the generated root `Schema::CEntityInstance` owns `_e`; the wrappers add only their `EntitySystem*` and semantic methods). Every generated accessor appears on the curated type with no hand-written forwarding, and adding a field to the manifest reaches plugins with zero wrapper edits. What stays hand-written in the wrappers is exactly what is semantic: `IsAlive()`, `Origin()`/`Angles()` (navigation through `BodyComponent().SceneNode()`), `EyePosition()`, `Slot()`, `Move()/SetMove()` (two fields), `ModelName()`, `SetMoney()` (now one chain-notified call), `Is*` team/observer predicates.

Migrate:

- `Entity.hpp:93-101`, `Pawn.hpp:30-51`, `Controller.hpp:36` → delete the `Field` members; the same names now exist as generated `Health()`/`SetHealth()` etc. Plugin call sites change from `pawn.Health = 100` to `pawn.SetHealth(100)` (section 7 lists the files).
- `Entity.cpp:18-178` → the file-scope statics disappear; `Origin()` becomes `BodyComponent().SceneNode().AbsOrigin()`, `SetMoney` becomes `InGameMoneyServices().SetAccount(v)` (chain-notified — the hand-rolled outer-field dirty disappears).
- `EntitySystem.cpp:154-180` → `pawn.MovementServices().Buttons().ButtonState(index)`.
- `Items.cpp:13`, `Transmit.cpp:25-29` (weapons/wearables `CUtlVector` handle views) → generated accessors returning the existing `HandleVectorView` through the `:HandleVectorView` manifest suffix.
- `Vote.cpp:34-39` + `Vote.hpp:110`, `Movement.hpp:64-87`, `EntitySystem.hpp:97`, `Items.hpp:50` (stored `SchemaPtr`s) → store the raw pointer or `EntityRef` and construct the generated view where used.
- `Render.cpp:10-11` → the duplicate declarations vanish; `SetRender` uses the inherited accessors.
- `UiFields.cpp:17-22,194-208` → `Schema::CCSCustomHudLayout` / `CCSCustomHudLayoutState`; the fabricated-FieldRef write becomes `layout.GlobalLayoutState().SetInputCaptureEnabled(v)` with the embedded-struct offset baked by the generator.
- `plugins/ui-lab/src/Commands.cpp:28,83` → `Schema::CCSCustomHudLayout{entity.Raw()}.Layout()`.

Docs: rewrite `docs/sdk/entities.md` around generated views + curated wrappers; `docs/sdk/gamedata.md` — gamedata says where functions/interfaces are, `schema/` says where fields are, both baked, both verified at load; consumer `CLAUDE.md` plugin-structure bullets updated (schema fields are `Schema::Class` accessors; regenerate on update).

Verify: build/test/lint; consumer build; live: godmode/slap/vitals (admin-system), bhop velocity/buttons, anticheat eye angles/origin, vote panel, custom HUD input capture, money HUD — every migrated site exercised once. `--relock`, commit voltmod then consumer.

## 7. Field inventory (seed for the manifest)

CBaseEntity: m_iHealth, m_iTeamNum, m_lifeState, m_fFlags, m_vecAbsVelocity, m_MoveType, m_nActualMoveType, m_hGroundEntity, m_CBodyComponent. CBaseModelEntity: m_vecViewOffset (as Vector), m_nRenderMode, m_clrRender. CBasePlayerController: m_iszPlayerName (char[128]), m_hPawn. CBasePlayerPawn: m_pObserverServices, m_pItemServices, m_pMovementServices, m_pWeaponServices, m_hController. CBaseCombatCharacter: m_hMyWearables. CBodyComponent: m_pSceneNode. CCSPlayerController: m_hPlayerPawn, m_pInGameMoneyServices. CCSPlayerController_InGameMoneyServices: m_iAccount. CCSPlayerPawn: m_ArmorValue, m_angEyeAngles, m_flVelocityModifier, m_bOnGroundLastTick. CCSPlayerPawnBase: m_flFlashDuration, m_flFlashMaxAlpha. CGameSceneNode: m_vecAbsOrigin, m_angAbsRotation. CInButtonState: m_pButtonStates. CModelState: m_ModelName. CPlayer_MovementServices: m_nButtons. CPlayer_ObserverServices: m_iObserverMode, m_hObserverTarget. CPlayer_WeaponServices: m_hMyWeapons. CSkeletonInstance: m_modelState. CVoteController: all. CCSCustomHudLayout: m_vecPanelIds, m_vecClassNames, m_vecDialogVariableNames, m_globalLayoutState, m_vecPlayerLayoutStates, m_strLayout. CCSCustomHudLayoutState: m_bInputCaptureEnabled.

Plugin call sites that change syntax (`X = v` → `SetX(v)`): admin-system `Admin/Actions/Vitals.cpp`, `Movement.cpp`, `Slap.cpp`, `Admin/Effects/Hide.cpp`, `Model.cpp`, `Disco.cpp`, `Fun/FunMode.cpp`, `Admin/CheatCheck/CheatCheckManager.cpp`; bhop `BhopManager.cpp:186-218`; anticheat `Correlation/ShotCorrelator.cpp`, `Simulator/CheatSimulator.cpp`, `AntiCheatManager.cpp:305`.

## 8. Review resolutions

Kept from earlier reviews: dedicated dev-plugin dumper; manifest-scoped generation (never the whole schema); plain-C++ generated headers; setters only where the dirty route is known; committed generated output; golden tests + live verification. Later simplifications: curated wrappers inherit the generated types instead of forwarding; no `diff` subcommand (`git diff` of generated code is the drift report); one `Generated.cpp`; committed IR stripped to what the generator reads; flat manifest with a `:CppType` suffix as the only override.

Superseded by the build-level decision: runtime resolution, `FieldRef` ABI pinning, the contract-table registry, transactional cache seeding, `SchemaSystemIsReady()` gating of lazy lookups — none exist any more. Their one surviving descendant is the load-time layout verification in section 5. Rejected outright: a write-policy enum / networked flag (unknowable per `85a526d`; writes always notify).

## 9. Risks

- **Update churn**: any layout shift in a used class = dump, `schemagen`, review `git diff`, rebuild, redeploy. The verification stage guarantees a stale build refuses to load rather than corrupting memory; keep the dump plugin installed on the local server so update day is one command.
- Verification timing: confirm `SchemaSystemIsReady()`/scope availability at Metamod load; if not always ready, the stage defers to `StartupServer` and the plugin must not touch schema before then (it already doesn't — entities come from map load).
- `CUtlTSHash` iteration API differs per SDK revision — pin against the conan package headers.
- Multi-base schema classes and unmapped types — flatten / skip with comments; never fail generation for them.
- modgraph doc-sync test — edit both fenced blocks; CRLF — always `\n`, check `.gitattributes`.
- Migration is the largest phase; do it wrapper by wrapper with a build between each, not in one sweep.

## 10. Commands

```bash
# vendor/voltmod
uv run poe build && uv run poe test && uv run poe lint
uv run voltmod schemagen --dump <path-to-server.json>
# consumer (editable voltmod registered)
uv run poe build
uv run poe build --install <plugin> --start        # live checks
uv run poe build --relock                          # then commit voltmod, then consumer + conan.lock
# update day: in-game `schema_dump` -> schemagen -> git diff -> build -> deploy
```
