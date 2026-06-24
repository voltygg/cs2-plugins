# admin-system + cs2-kit — Refactor & DX Review

> Scope: a refactoring/DX review of both codebases (`src/**` and `vendor/cs2-kit/{include,src}/**`),
> targeting duplication, boilerplate, accidental complexity, and churn — with the explicit goal of
> **lowering the C++ skill floor** so less-experienced devs can build plugins. Plus decisions on SQL
> migrations, build/library management, and the cs2-kit ↔ plugin API boundary.
>
> Method: 18-agent review (7 subsystems × find→adversarial-verify, 3 decision tracks, 1 completeness
> critic). Every claim below carries `file:line` refs. **74 findings survived adversarial verification**
> (13 high / 32 medium / 29 low); **41 are tagged "belongs in cs2-kit"** — i.e. more than half the
> boilerplate is generic and should not live in the plugin at all.

---

## Executive summary

The architecture is sound and the recent data-descriptor refactor (Actions/Effects) is the *right*
direction. The problems are not design flaws — they are **repetition and a stale library boundary**:

1. **Error-swallowing is the codebase default.** ~30 repository methods (and more) are
   `try { … } catch (const std::exception&) { return default; }` that drop `e.what()`. Failures are
   invisible; this is the single biggest DX/debugging hazard.
2. **The same concept is implemented 2–3× and drifts.** Target resolution exists three times
   (commands / menus / actions) even though cs2-kit already ships `StringUtils::ParseTarget`. Permission
   checks split into `HasPermission(char)` vs `HasAnyPermission(string)`. Punishment issue/remove paths
   are near-verbatim triplets.
3. **Per-feature file tax.** 16 action/effect descriptors across ~22 tiny files; 5 repositories that are
   CRUD twins (VoiceMute/TextMute are byte-for-byte); identical `using`-blocks copied per command file.
4. **The docs teach APIs that no longer exist.** Both `CLAUDE.md` files + `docs/sdk.md` still teach
   `CRTP Singleton` / `::Instance()` — there is **no `Singleton.hpp`** and **zero `::Instance()` call
   sites**. The first thing a newcomer reads is wrong.
5. **Generic infrastructure is stranded in the plugin** (HttpClient, EscapeHtml/center-HTML scaffolding,
   the whole descriptor/dispatch engine) while **unused "reusable" code sits in the kit**
   (`MenuBuilder::WithContext` was built for this exact problem and is never called).
6. **No safety net.** Zero tests; no static tie between an `EffectId` and its descriptor; a raw
   `Execute(sql)` injection footgun; `migrations` table exists but no runner applies it.

**Headline recommendation:** spend the next cycle moving boilerplate *down* into cs2-kit (DB query
wrapper, target resolver, menu rows, plugin-services holder), fix the docs, and add the missing
migration runner — each promotion shipped **together with its admin-system migration** so you don't
create more dead abstractions.

---

## Cross-cutting themes

| # | Theme | Representative findings |
|---|-------|------------------------|
| T1 | Error-swallowing as the default (try/catch-return-default + silent UI no-ops) | DAT-1, DAT-2, SUP-3, COR-6, SUP-11 |
| T2 | Same concept implemented 2–3× and drifting (target-resolve, permission-check, godmode) | COM-2, COM-3, COM-4, COM-8, MEN-1, ACT-9 |
| T3 | Per-feature file/descriptor boilerplate that should collapse to one table/base | ACT-1, DAT-3, DAT-4, SUP-2, DAT-5, COM-10, MEN-11 |
| T4 | Docs teach dead APIs (Singleton/`::Instance`, wrong PlayerController ctor) | COR-1, SDK-1, SDK-8, COR-4, SDK-6 |
| T5 | Dual ownership model (`Kit()` vs `Sys()`) + dead/orphaned plumbing | COR-2, COR-3, COR-4, COR-5, DAT-6, ACT-10 |
| T6 | Reusable infra stranded in plugin; unused "reusable" code in kit | SUP-4, SUP-9, ACT-5, MEN-3, SDK-2 |
| T7 | No safety net: no tests, no enum↔descriptor assert, no SQL/permission compile gate | ACT-6, MEN-10, DAT-2, COM-11 |

---

## Prioritized roadmap

Ordered by impact-per-effort. Each item lists the findings it closes. "LIB" = lands in cs2-kit.

### Phase 0 — Free wins (low effort, high trust)
1. **Fix the docs that teach dead APIs.** Purge `CRTP Singleton`/`::Instance()` from both `CLAUDE.md`
   and `docs/sdk.md`; also fix the wrong `PlayerController` ctor, the phantom `SetPlayerName`
   scoreboard re-sync (SDK-8), and the `m_clrRender` byte-order note. Align everything to
   `Services`/`Kit()` + `Managers`/`Sys()`. *(COR-1, SDK-1, SDK-8, COR-4)* — **also fixes the
   `CLAUDE.md` project-instruction that currently tells contributors to follow the dead pattern.**
2. **Delete dead code:** `PlayerRecord` entity (DAT-8), `BuildDurationMenu` wrapper (MEN-7),
   `EffectId::Blind` row+key (ACT-6), the dead `teamOnly` branch in `RebroadcastAdminChat` (SUP-10),
   the misleading `Database` lifecycle methods (DAT-8), and the dead `Database` mutex (DAT-7).
3. **Fix doc/script drift** flagged by the build track: `--vs-version 18` vs `19`, the `admins.json`
   ghost, stale `build/` paths in `.vscode/tasks.json`, the missing `generate-protos.sh` /
   `init-db.sql` referenced by docs+compose. *(verify each before scheduling — see Gaps)*

### Phase 1 — The error-swallowing + DB layer (highest leverage)
4. **One DB query wrapper, `std::expected`-based, in cs2-kit.** `DbResult<T> = std::expected<T,std::string>`
   + a single `TryDb(lambda)`/`Db::Query(...)` that owns the try/catch **and logs scrubbed messages
   (no DSN/password — see Bugs)**. Collapse all repository methods onto it. Make `Execute()`
   constant-SQL-only; route param-less reads through the prepared path. *(DAT-1, DAT-2, SUP-3, T1)* — LIB
5. **Fix `ExecutePrepared`:** hold one long-lived connection (main-thread-only → safe), prepare each
   named statement once, use `pqxx::nontransaction`/`read_transaction` for SELECTs. Today it opens a
   fresh connection **and re-prepares on every call** — the "prepared statement" buys nothing. *(DAT-2)* — LIB
6. **Collapse the repository CRUD shape.** A `Db::Repository` base (FindById/Delete/wrapper — **NOT an
   ORM**) and merge VoiceMute/TextMute into one parameterized mute repo. Keep each entity's `ParseRow`
   in the plugin. *(DAT-3, DAT-4, DAT-5, SUP-2)* — LIB

### Phase 2 — Unify the duplicated concepts
7. **One `TargetResolver` service in cs2-kit**, built on the existing `StringUtils::ParseTarget`
   (currently dead-code; `TargetResolver` ignores it). Inject `HasPermission`/`CanTarget` callbacks so
   no admin concept leaks into the kit. Reimplement `ActionContext::Resolve`,
   `CommandHelpers::ResolveSingle`, and the inline resolve in `MakePunishmentCallback` on top of it.
   **Filter blocked targets at the `PlayerPicker` source** so immune targets aren't even offered, and
   report a reason on no-op (fixes the silent menu UX). Adds `@all/@me/@team`. *(COM-2, COM-8, MEN-1, T2)* — LIB
8. **One permission predicate + typed enum end-to-end.** Replace free-form `Command.Permission` strings
   and raw char literals with `Permission`; unify `HasPermission`/`HasAnyPermission` so command and
   action paths can't diverge. *(COM-3, COM-4, T2)*
9. **Unify the punishment issue/remove skeletons** behind one templated path. *(SUP-1, SUP-5)*

### Phase 3 — Menu + effect ergonomics
10. **A permission-gated, auto-closing "player-action row"** menu helper that fuses
    label + permission gate + resolve + `CloseAllMenus`, deriving the gate from the action descriptor's
    `Flag`. Retire the dead `WithContext`/`AddContextButton` (or adopt them — but don't leave them
    unused). *(MEN-1, MEN-2, MEN-3, MEN-6, MEN-8, MEN-10, T6)* — LIB
11. **Promote `BuildPlayerPicker` + the duration picker (+ `ParseDuration`)** into cs2-kit as
    content-agnostic builder rows (labels passed as params), then migrate `BuildControlMenu`'s
    hand-rolled loop onto them. *(MEN-4, MEN-11, MEN-12)* — LIB
12. **Adopt `ScheduledEffect` for Disco** (or give `EffectSetup` a `DurationMs` owned by
    `EffectManager`). Fixes the orphan-timer slot-aliasing **bug** and the Launch-clears-Godmode **bug**
    in one move. *(COR-3, ACT-4, ACT-9 — see Bugs)*
13. **Collapse the ~22 descriptor files into one registry table** binding `EffectId`→descriptor so a
    missing descriptor is a compile/load error (kills the dead-column footgun). Use designated
    initializers. *(ACT-1, ACT-2, ACT-5, ACT-7)* — LIB

### Phase 4 — Boundary + infra promotion
14. **`PluginServices<T>` holder in cs2-kit** so plugin #2 doesn't hand-roll `Managers`/`Sys()`; codify
    a standard `Defer()` teardown. *(COR-2, COR-5, COR-7)* — LIB
15. **Promote `HttpClient` (+ `HttpResult`) and `EscapeHtml`/center-HTML scaffolding** into cs2-kit;
    repoint CheatCheck. *(SUP-4, SUP-9)* — LIB **(verify libcurl completions marshal to the game thread
    first — see Gaps)**
16. **SDK ergonomics:** cache the resolved pawn in `PlayerController` (SDK-3), unify the two
    pawn-resolution paths (SDK-4), use the kit's own `EntityRender`/`Slot`/`ObserverMode` constants
    instead of re-hardcoding (SDK-2, SDK-5, SDK-6), dedupe `EntitySystem` offset cache + resolution
    (SDK-9, SDK-10). *(SDK-2..SDK-11)* — mostly LIB

### Phase 5 — Safety net
17. **Stand up doctest** with a first suite (ParseTarget / SteamId / duration parsing / permission
    bitmask / CheatCheckMode), a load-time **translation-key coverage check**, and **GitHub Actions CI**
    (build via the existing Docker image + ruff + clang-format). *(T7)*
18. **The migration runner** — see Decision 1.

---

## Decision 1 — SQL migrations

**Recommendation: build a tiny in-plugin embedded migration runner in cs2-kit
(`CS2Kit::Database::Migrator`).** On load it applies an ordered list of versioned `.sql` files, one
transaction each, advisory-lock-guarded, recording applied versions in a `schema_migrations` table.

Why this and not the alternatives:

- **Current state is half-built and undocumented-in-code.** `database/schema.sql:174` already declares a
  `migrations` table and seeds version 1 (`:257`), but **nothing in `src/**` reads or applies it** —
  `Database::Initialize` only opens a connection, and `deploy.sh` ships only `configs/` + binaries, so
  `database/` never even reaches the server. Operators run `psql` by hand.
- **External tools (dbmate / golang-migrate / Flyway / Liquibase / sqitch / Atlas) are the wrong
  shipping model.** The product is one `.dll` + a config folder for server operators who are *not* DB
  experts. Forcing them to install Go/Java + a CLI and remember to run it on every update is exactly the
  burden the DX goal forbids — and the plugin still can't guarantee it ran.
- **Idempotent single-schema-on-load** is tempting (schema.sql is ~90% idempotent already) but degrades
  badly: type changes, renames, NOT NULL additions, backfills need hand-written `DO $$…$$` guards that
  newcomers get wrong, and you lose any record of applied state.
- **Embedding SQL as C++ string literals** loses `.sql` tooling and forces a rebuild per migration.

It belongs **in cs2-kit** (every future plugin gets migrations for free) and reuses primitives that
already exist (`Core::ResolvePath`, the `Json::TryDeserializeFile` try/log idiom). ~120–150 LOC,
no threads, no new mutex (the re-entrancy guard is `pg_advisory_xact_lock`, a DB-side lock).

**Implementation sketch:**
1. Move the schema to `configs/migrations/0001_initial_schema.sql`, `0002_*.sql`, … (under `configs/`
   so the existing `deploy.sh` subdir loop copies it automatically).
2. `bool Migrate(pqxx::connection&, const std::string& dir)`: `CREATE TABLE IF NOT EXISTS
   schema_migrations`; `pg_advisory_xact_lock(<const>)`; read `max(version)`; enumerate dir via
   `std::filesystem`, sort by parsed leading integer, filter `> current`; for each pending file, one
   `pqxx::work` → `exec(contents)` → insert version → commit.
3. Wire into `ConnectDatabaseAndLoadAdmins()` **before** `LoadGroups()`/`LoadAdmins()`; on failure
   `Log::Warn` + return false (degrade safely, don't query a stale schema).
4. Re-entrant on hot reload by construction (max-version short-circuit + xact-scoped lock). Forward-only;
   rollback = restore backup. Contributors add a migration by **dropping a numbered `.sql` file** — no
   C++ touched.

> Caveat to verify: confirm `deploy.sh`'s subdir loop actually recurses into `configs/migrations/`; if
> not, add one line. Make the runner ignore non-`NNNN_*.sql` filenames so stray `*.sql.bak` aren't run.

---

## Decision 2 — Build, libraries, project structure, onboarding

**Recommendation: keep AMBuild** (it's the AlliedModders HL2SDK standard and your `AMBuildScript`
already does the hard SDK/manifest wiring correctly — rewriting it in CMake/xmake means
reimplementing HL2SDK manifest logic by hand, high risk for zero end-user gain). The leverage is **not**
the build tool; it's onboarding friction and error ergonomics. Do these additive changes:

- **`scripts/bootstrap.sh`** — one command: `git submodule update --init --recursive --depth 1` →
  `uv sync` → `vcpkg install` → proto-gen → `build.sh`. Collapses the 6-step README ritual.
- **Generate `compile_commands.json`** (AMBuild `--gen=compile_commands`) and **switch to clangd**:
  remove the clangd block from `.vscode/extensions.json` unwanted list, add a checked-in `.clangd`, then
  delete the 90-line hand-maintained `.vscode/c_cpp_properties.json` that silently drifts from
  `src/AMBuilder`.
- **CI** (`.github/workflows/ci.yml`): build via the existing Docker image + `ruff check` +
  `clang-format --dry-run --Werror`. There is currently **no CI** under admin-system.
- **`CONTRIBUTING.md`** stating the hard constraints up front (main-thread-only / no mutex except
  Database, `.hpp` + C#-naming, `std::format`, "drop a `.cpp` under `src/` and it's auto-discovered").
- **Fix the doc/script drift** (Phase 0, item 3).

**Libraries worth adopting (all header-only, all shrink boilerplate):**

| Lib | Replaces | Note |
|-----|----------|------|
| `std::expected` (C++23) | the 42 swallow-everything DB try/catch blocks | scoped to the DB layer; **not** a project-wide `Result<T>` |
| `magic_enum` | hand-written `ParseMode`/enum↔string ladders (e.g. `CheatCheckMode.hpp:15`) | keep out of hot per-frame paths; mind the −128..127 default range |
| `doctest` | (nothing — there are **zero** tests) | second binary that links pure-logic units *without* the SDK |

**Keep `std::format`** — do **not** add fmtlib (redundant on C++23/MSVC). **Pin SDK submodule SHAs** and
add `scripts/update-sdks.sh` so a clone is reproducible and an SDK bump is a reviewable commit.

---

## Decision 3 — cs2-kit ↔ plugin boundary & the ownership duality

**The "duality" is mostly already resolved — it's a documentation ghost.** cs2-kit's `Services`/`Kit()`
and admin-system's `Managers`/`Sys()` are the *same* model ("one container built on Load, destroyed on
Unload, declaration-order = construction-order"). The `CRTP Singleton` in the docs **doesn't exist in
code** (`Glob` finds no `Singleton.hpp`; `Grep` finds zero `::Instance()` uses). So:

- **Kill the Singleton docs**, bless the container model, and **promote a thin `PluginServices<T>`
  holder** into cs2-kit so plugin #2 copies 5 lines instead of 60. Keep `Kit()` (kit services) and
  `Sys()` (plugin services) as two *names with one shape* — different scopes, not a duality to merge.

**Promote four low-magic helpers (each shipped with its admin-system migration):**
1. `Db` query wrapper + connection class (Decision 2 / Phase 1) — **not** a Repository<TEntity> ORM.
2. `Players::TargetResolver` with injected permission/immunity callbacks (Phase 2).
3. Menu convenience rows: player picker, duration picker, confirm — content-agnostic (labels as params).
4. A thin permission-aware command-registration overload (string-typed, so the kit stays admin-agnostic).

**Do NOT** build: a unified project-wide `Result<T>`, an entity-mapping/ORM layer, or a generic
manager-registry with auto dependency ordering — those add abstraction cost that *raises* the skill
floor, the opposite of the goal.

**The target:** a documented ~30-line minimal plugin (derive `MetamodPluginBase` → `Info()` → `OnLoad`
that builds `Managers` + registers one command via the helper + one menu via `AddPlayerPicker`). Ship it
as a copy-paste starter so the boilerplate-removal is self-reinforcing.

> **Guardrail (the most important lesson here):** `MenuBuilder::WithContext`/`AddContextButton` were
> added to the kit for exactly this and **adopted nowhere** — an unmigrated helper is worse than none.
> Every promotion lands with its consumer migration in the same change.

---

## Reconciling the three contradictions the review surfaced

The decision tracks contained guidance that reads as contradictory; the precise lines:

1. **Repository base vs "no ORM":** a CRUD-*shape* base (FindById/Delete/try-catch wrapper) is in scope;
   reflection/auto-mapping of columns→fields is **out**. `ParseRow` stays per-entity in the plugin.
2. **`std::expected` vs "no `Result<T>`":** `std::expected` **scoped to the DB layer** = yes; a single
   blessed `Result<T>` threaded through every API (DB + commands + actions) = no.
3. **Singleton:** the docs fix (item 1) is a **hard prerequisite** for the ownership decision, not a
   parallel nice-to-have — the current `CLAUDE.md` actively instructs contributors to follow the dead
   pattern.

---

## Correctness bugs found en route

These are real bugs (not style), surfaced while reviewing for refactors — worth fixing regardless:

- **Disco orphan-timer slot-aliasing** (`Disco.cpp:49`): the 15s auto-cancel `Delay` handle is discarded,
  so it survives an early cancel and can clobber a freshly re-applied effect on the same slot. Fixed by
  adopting `ScheduledEffect`/`EffectManager`-owned duration (roadmap item 12).
- **Launch clears the admin's Godmode** (ACT-9): Godmode exists as *both* an Action and a flag-toggle;
  the two mechanisms collide. Unify to one (item 12/13).
- **Credential leak risk** (gap): `GetConnectionString()` (`Database.cpp:9`) embeds the password into a
  process-lifetime string. The moment error logging starts echoing libpqxx `e.what()` (Phase 1!),
  connection-failure messages can leak the full DSN. **The logging refactor MUST scrub connection
  strings** — this is a security constraint on the highest-impact recommendation.
- **`SetPlayerName` doc bug** (SDK-8): `docs/sdk.md:113` claims it issues `NetworkStateChanged` to
  re-sync the scoreboard; the code does `memset`+`memcpy` only.
- **Input parsing fragility** (gap): `ParseArguments` → `StringUtils::Split(' ')` yields empty tokens on
  double-spaces, so `!ban  Bob` silently fails; no quote handling, so names with spaces are impossible
  from chat. Trim/skip empties in the targeting refactor (item 7).

---

## Gaps / explicitly not covered (decide before scheduling)

- **`audit_log` is unimplemented dead schema** — table exists in `schema.sql`, but there is **no
  `AuditLogRepository` and no writes anywhere**. For an admin plugin this is a real feature/security gap.
  Decide: implement it (natural first consumer of the Repository base + transaction wrapper) or delete it
  from schema+docs.
- **Transaction atomicity:** a logical "ban" is ban + (intended) audit + cache as independent
  connection-per-query ops — not atomic. Once Phase 1 gives a persistent connection, wrap a logical
  punishment in one `pqxx::work`.
- **Thread-safety of HttpClient promotion (SUP-4):** verify libcurl completions truly marshal back to the
  game thread before touching managers, and whether removing the `Database` mutex is safe given the
  (currently fictional) async ambition.
- **Build-track drift claims** (`generate-protos.sh`, `init-db.sql`, `admins.json`, `--vs-version`) were
  taken from the build agent and **not independently re-verified** in this pass — confirm each before it
  drives `CONTRIBUTING.md`/`bootstrap.sh`.
- **i18n robustness / config-error DX** were under-reviewed (no check that `en`/`ru` are key-complete;
  unclear whether config errors report *which* key failed).

---

## Appendix: full findings (74, adversarially verified)

Legend: `SEV` severity · `V` verdict (c=confirmed, p=partial) · `LIB` belongs in cs2-kit · `KIND`.



### Database & Persistence

| ID | SEV | V | LIB | KIND | Finding | Key locations |
|----|-----|---|-----|------|---------|---------------|
| DAT-1 | H | c | Y | boilerplate | Per-method try/catch that swallows std::exception is copy-pasted across every repository method | `src/Database/Repositories/BanRepository.cpp:15-32 src/Database/Repositories/BanRepository.cpp:60-77` |
| DAT-2 | H | c | Y | complexity | ExecutePrepared is connection-per-query + redundant prepare; the 'prepared statement' name is dead ceremony | `src/Database/Database.hpp:54-68 src/Database/Database.cpp:56-73` |
| DAT-3 | H | c | Y | duplicate | ParseRow row->entity mapping is hand-duplicated per entity and per repository | `src/Database/Repositories/BanRepository.cpp:176-199 src/Database/Repositories/VoiceMuteRepository.cpp:120-140` |
| DAT-4 | M | c |  | duplicate | TextMute and VoiceMute entities + repositories are near-verbatim duplicates | `src/Database/Entities/TextMute.hpp:9-28 src/Database/Entities/VoiceMute.hpp:9-28` |
| DAT-5 | M | p | Y | structure | Generic Repository<TEntity> base / free query helpers to kill the remaining CRUD shape duplication | `src/Database/Repositories/BanRepository.cpp:13-78 src/Database/Repositories/VoiceMuteRepository.cpp:13-47` |
| DAT-6 | M | c |  | boilerplate | Repositories are stateless but reconstructed ad-hoc at every call site | `src/Punishments/PunishmentManager.cpp:63-65 src/Punishments/PunishmentManager.cpp:159` |
| DAT-7 | M | c |  | complexity | Database mutex contradicts the documented main-thread-only / no-mutex design and guards nothing meaningful | `src/Database/Database.hpp:4 src/Database/Database.hpp:51` |
| DAT-8 | L | c |  | churn | Dead code: PlayerRecord entity and misleading Database lifecycle methods | `src/Database/Entities/Player.hpp:9-22 src/Database/Database.cpp:45-49` |
| DAT-9 | L | p | Y | dx | Schema applied entirely out-of-band; no migration/bootstrap path in the plugin | `src/Database/Database.cpp:15-43 src/Core/Plugin.cpp:101-119` |

### Admin Actions & Effects

| ID | SEV | V | LIB | KIND | Finding | Key locations |
|----|-----|---|-----|------|---------|---------------|
| ACT-1 | H | c |  | boilerplate | 16 descriptors scattered across ~22 tiny files; headers are pure extern-const boilerplate | `src/Admin/Actions/Vitals.hpp:8-11 src/Admin/Actions/Movement.hpp:8-11` |
| ACT-2 | M | c | Y | duplicate | Three Run() variants duplicate the resolve -> Valid -> RequireAlive prologue | `src/Admin/Actions/ActionContext.cpp:54-63 src/Admin/Actions/ActionContext.cpp:65-74` |
| ACT-3 | M | p | Y | structure | Swap / CallCheck / CancelCheck bypass the descriptor model and hand-roll the guard | `src/Admin/Actions/Teleport.cpp:31-50 src/Admin/Actions/CheatCheck.cpp:10-26` |
| ACT-4 | M | c | Y | duplicate | Cancel/timer closures repeat the PlayerController revalidation dance in 5 places | `src/Admin/Effects/Ghost.cpp:16-20 src/Admin/Effects/Disco.cpp:41-42` |
| ACT-5 | M | p | Y | structure | Descriptor + dispatch + EffectManager are generic but trapped in admin-system | `src/Admin/Actions/ActionContext.hpp:18-66 src/Admin/Effects/EffectAction.hpp:21-30` |
| ACT-10 | L | p |  | complexity | ActionContext stores Admin/Target pointers AND AdminCtrl/TargetCtrl controllers redundantly | `src/Admin/Actions/ActionContext.hpp:18-26 src/Admin/Actions/ActionContext.cpp:21-25` |
| ACT-6 | L | c |  | churn | EffectId::Blind is dead: declared, translated, and a permanently-disabled menu row | `src/Admin/Effects/EffectId.hpp:13 src/Admin/Menu/AdminMenu_Effects.cpp:59` |
| ACT-7 | L | c |  | dx | Positional ctors with /*requireAlive*/ comment-tags instead of designated initializers | `src/Admin/Actions/Vitals.cpp:8 src/Admin/Actions/Movement.cpp:23-40` |
| ACT-8 | L | c | Y | complexity | RoundScoped duplicated across EffectSetup and ActiveEffect; Apply takes an exploded param list | `src/Admin/Effects/EffectManager.hpp:18-32 src/Admin/Effects/EffectManager.hpp:50` |
| ACT-9 | L | p | Y | structure | Godmode lives as both an Action and a flag-toggle, splitting one concept across two mechanisms | `src/Admin/Actions/Vitals.cpp:13-19 src/Admin/Menu/MenuHelpers.hpp:45-58` |

### Menu System

| ID | SEV | V | LIB | KIND | Finding | Key locations |
|----|-----|---|-----|------|---------|---------------|
| MEN-1 | H | c |  | duplicate | Hand-written kick/warn/punish callbacks re-fetch the player pair instead of reusing the existing ActionContext resolver | `src/Admin/Menu/AdminMenu_Punish.cpp:193-208 (kick) src/Admin/Menu/AdminMenu_Punish.cpp:242-260 (warn)` |
| MEN-2 | H | c |  | boilerplate | Add an admin-menu "player-action row" helper that fuses label + permission gate + resolve + auto-close | `src/Admin/Menu/AdminMenu_Punish.cpp:193-260 src/Admin/Menu/MenuHelpers.hpp:26-58` |
| MEN-3 | H | c | Y | churn | Dead cs2-kit helpers WithContext/AddContextButton — built for this exact problem, never used, and don't fit | `vendor/cs2-kit/include/CS2Kit/Menu/MenuBuilder.hpp:51-72 src/Admin/** (zero call sites — see grep)` |
| MEN-4 | M | c |  | duplicate | BuildControlMenu duplicates BuildPlayerPicker's player-list logic just to prepend one toggle | `src/Admin/Menu/AdminMenu_Control.cpp:34-71 src/Admin/Menu/PlayerPicker.cpp:17-40` |
| MEN-5 | M | p | Y | duplicate | ChoiceOption and SelectorOption are ~90% duplicate code | `vendor/cs2-kit/include/CS2Kit/Menu/Options/ChoiceOption.hpp:18-92 vendor/cs2-kit/include/CS2Kit/Menu/Options/SelectorOption.hpp:18-91` |
| MEN-6 | M | p |  | boilerplate | Repeated admin/target resolve-and-gate prologue at the top of every actions-menu builder | `src/Admin/Menu/AdminMenu_Punish.cpp:174-191 src/Admin/Menu/AdminMenu_Control.cpp:73-90` |
| MEN-7 | M | c |  | churn | Dead code: BuildDurationMenu wrapper has no callers | `src/Admin/Menu/AdminMenu_Punish.hpp:15-16 src/Admin/Menu/AdminMenu_Punish.cpp:158-162` |
| MEN-8 | M | c |  | boilerplate | CloseAllMenus(slot) is copy-pasted onto every terminal callback; easy to forget, no enforcement | `src/Admin/Menu/AdminMenu_Punish.cpp:110,126,206,258 src/Admin/Menu/PresetSubmenu.cpp:28` |
| MEN-9 | M | c | Y | complexity | ChatSettings color/language pickers use a fragile shared_ptr<int> 'pending index' dance | `src/Admin/Menu/AdminMenu_ChatSettings.cpp:111-138 (AddColorChoice) src/Admin/Menu/AdminMenu_ChatSettings.cpp:164-192 (AddLanguageChoice)` |
| MEN-10 | L | p |  | boilerplate | Per-row CanActOn permission lookups computed eagerly and scattered, not derived from the action data | `src/Admin/Menu/AdminMenu_Control.cpp:86-117 src/Admin/Menu/AdminMenu_Effects.cpp:52-61` |
| MEN-11 | L | c |  | boilerplate | BuildTimedPunishmentMenu re-copies target/callback per row and reimplements the duration table inline | `src/Admin/Menu/AdminMenu_Punish.cpp:85-133` |
| MEN-12 | L | c |  | boilerplate | Swap row hand-rolls a nested player-picker + close; the 'pick a second player then run' flow should be a reusable helper | `src/Admin/Menu/AdminMenu_Effects.cpp:63-75 src/Admin/Menu/AdminMenu_Punish.cpp:166-171` |

### Commands & Targeting

| ID | SEV | V | LIB | KIND | Finding | Key locations |
|----|-----|---|-----|------|---------|---------------|
| COM-2 | H | p | Y | duplicate | Three independent target-resolution implementations across commands, menus, and actions | `src/Commands/TargetResolver.cpp:19-53 src/Commands/CommandHelpers.cpp:25-55` |
| COM-1 | M | c | Y | duplicate | Every punishment handler repeats the same ResolveSingle target-guard preamble | `src/Commands/PunishmentCommands.cpp:35-40 src/Commands/PunishmentCommands.cpp:50-54` |
| COM-3 | M | c | Y | boilerplate | Command.Permission is a free-form string while the plugin has a typed Permission enum | `vendor/cs2-kit/include/CS2Kit/Commands/Command.hpp:25 src/Core/Permissions.hpp:30-33` |
| COM-4 | M | c |  | churn | Raw permission char literals bypass the Permission enum in four command files | `src/Commands/InfoCommands.cpp:67 src/Commands/InfoCommands.cpp:76` |
| COM-8 | M | p | Y | dx | Targeting only supports #slot and name-substring; no @all/@me/@team selectors | `src/Commands/TargetResolver.cpp:37-52 src/Commands/TargetResolver.hpp:18-28` |
| COM-10 | L | c |  | boilerplate | Identical using-declaration header block copied into every command .cpp | `src/Commands/PunishmentCommands.cpp:17-28 src/Commands/InfoCommands.cpp:17-20` |
| COM-11 | L | p |  | complexity | Unban/cc handlers re-validate args the WithArgs/dispatcher contract should guarantee | `src/Commands/PunishmentCommands.cpp:76-90 src/Commands/PunishmentCommands.cpp:57-61` |
| COM-5 | L | c | Y | complexity | MaxArgs = 99 magic sentinel for 'unbounded' | `vendor/cs2-kit/include/CS2Kit/Commands/Command.hpp:27 vendor/cs2-kit/include/CS2Kit/Commands/Command.hpp:62` |
| COM-6 | L | p | Y | boilerplate | Per-command WithUsage strings duplicate the name and WithArgs bounds | `src/Commands/PunishmentCommands.cpp:200-204 src/Commands/PunishmentCommands.cpp:208-212` |
| COM-7 | L | c | Y | boilerplate | Permission/result callback wiring is hand-rolled plugin glue every consumer will copy | `src/Core/Plugin.cpp:84-99 vendor/cs2-kit/include/CS2Kit/Commands/CommandManager.hpp:45-46` |
| COM-9 | L | c | Y | complexity | GetCommand does a redundant second linear scan after the hash lookup | `vendor/cs2-kit/src/Commands/CommandManager.cpp:81-96 vendor/cs2-kit/src/Commands/Command.cpp:9-20` |

### cs2-kit Core & Lifecycle

| ID | SEV | V | LIB | KIND | Finding | Key locations |
|----|-----|---|-----|------|---------|---------------|
| COR-1 | H | c | Y | churn | Documentation describes a Singleton<T>::Instance() API that no longer exists | `vendor/cs2-kit/docs/architecture.md:22 vendor/cs2-kit/docs/architecture.md:41-61` |
| COR-2 | M | p | Y | structure | Dual service-locator model (Kit() vs Sys()) every TU must learn and dual-include | `vendor/cs2-kit/include/CS2Kit/Core/Services.hpp:36-73 src/Core/Managers.hpp:20-34` |
| COR-3 | M | p |  | duplicate | ScheduledEffect is fully implemented but never used; Disco hand-rolls the same pattern | `vendor/cs2-kit/include/CS2Kit/Core/ScheduledEffect.hpp:24-46 vendor/cs2-kit/src/Core/ScheduledEffect.cpp:1-89` |
| COR-4 | M | c |  | churn | Dead lifecycle API left from the singleton->Services migration | `src/Core/Plugin.cpp:49-52 src/Core/Plugin.hpp:26-30` |
| COR-5 | M | p | Y | boilerplate | Every plugin repeats the same fixed Defer() teardown block for kit-owned state | `src/Core/Plugin.cpp:201-208 src/Core/Plugin.cpp:256-267` |
| COR-6 | M | c | Y | duplicate | ChatService mixes chat formatting with rate-limited mute-notice logic (duplicated) | `src/Core/ChatService.cpp:146-211 src/Core/ChatService.hpp:65-69` |
| COR-9 | M | c | Y | duplicate | Chat-command prefix/say-unquoting parsing duplicated between base hook and ChatService | `vendor/cs2-kit/src/Core/MetamodPluginBase.cpp:166-172 src/Core/ChatService.cpp:138-143` |
| COR-7 | L | p |  | complexity | OnLoad wiring is hand-ordered free functions reaching through global Sys()/Kit() | `src/Core/Plugin.cpp:69-147 src/Core/Plugin.cpp:164-212` |
| COR-8 | L | c | Y | structure | Three separate global-state holders (Logger, Paths, active Services) use ad-hoc file-static singletons | `vendor/cs2-kit/src/Core/Logger.cpp:6-16 vendor/cs2-kit/src/Core/Paths.cpp:6-17` |

### cs2-kit SDK Wrappers

| ID | SEV | V | LIB | KIND | Finding | Key locations |
|----|-----|---|-----|------|---------|---------------|
| SDK-1 | H | c | Y | churn | docs/sdk.md teaches a dead API (singletons + wrong PlayerController ctor) after the Services/Kit migration | `vendor/cs2-kit/docs/sdk.md:13-22 (GameInterfaces::Instance()) vendor/cs2-kit/docs/sdk.md:31-38 (GameData::Instance())` |
| SDK-2 | M | c |  | duplicate | EntityRender constants/enum re-hardcoded in the plugin instead of using the library's exports | `src/Admin/Effects/Disco.cpp:19-21 (RenderModeNormal=0, RenderModeTransTexture=3, ColorOpaqueWhite=0xFFFFFFFFu) src/Admin/Effects/Disco.cpp:44,54-55 (pc.SetRender(3, ...))` |
| SDK-3 | M | c | Y | complexity | PlayerController re-resolves the pawn (schema lookup + handle deref) on every typed field access | `vendor/cs2-kit/src/Sdk/PlayerController.cpp:58-69 (GetPawn does GetOffset + ResolveEntityHandle every call) vendor/cs2-kit/include/CS2Kit/Sdk/PlayerController.hpp:45-55 (GetPawnField calls GetPawn each time)` |
| SDK-4 | M | p | Y | duplicate | Two different pawn-resolution paths (m_hPawn vs m_hPlayerPawn) across EntitySystem and PlayerController | `vendor/cs2-kit/src/Sdk/Entity.cpp:26 (GetButtons path resolves CBasePlayerController::m_hPawn) vendor/cs2-kit/src/Sdk/Entity.cpp:130-158 (GetPlayerButtons re-implements pawn resolution by hand)` |
| SDK-10 | L | p | Y | boilerplate | EntitySystem keeps its own offset cache (_offsetButtons etc.) duplicating SchemaService's cache | `vendor/cs2-kit/include/CS2Kit/Sdk/Entity.hpp:64-71 (4 cached offset ints + _schemaOffsetsResolved flag + ResolveSchemaOffsets) vendor/cs2-kit/src/Sdk/Entity.cpp:19-32, 130-136` |
| SDK-11 | L | p | Y | boilerplate | PlayerController name read/write hand-rolls C-string buffer handling that a helper could centralize | `vendor/cs2-kit/src/Sdk/PlayerController.cpp:230-262 (GetPlayerName manual strlen loop, SetPlayerName manual memset/memcpy/clamp)` |
| SDK-5 | L | c | Y | boilerplate | MessageSystem hardcodes slot bound 64 instead of Core::MaxPlayers / Core::IsValidSlot | `vendor/cs2-kit/src/Sdk/UserMessage.cpp:86 (slot < 0 \|\| slot >= 64) vendor/cs2-kit/src/Sdk/UserMessage.cpp:114 (slot < 0 \|\| slot >= 64)` |
| SDK-6 | L | c | Y | dx | ObserverMode_t enum exists but accessors use raw uint8_t/int and have no callers (dead + inconsistent typing) | `vendor/cs2-kit/include/CS2Kit/Sdk/ObserverMode.hpp:12-19 (ObserverMode_t enum) vendor/cs2-kit/include/CS2Kit/Sdk/PlayerController.hpp:116-119 (GetObserverMode->int, SetObserverMode(uint8_t))` |
| SDK-7 | L | p | Y | complexity | PlayerController public header exposes 4 raw reinterpret_cast schema templates as the primary escape hatch | `vendor/cs2-kit/include/CS2Kit/Sdk/PlayerController.hpp:34-78 (GetField/GetPawnField/SetField/SetPawnField)` |
| SDK-8 | L | c | Y | bug | SetPlayerName: docs claim a NetworkStateChanged scoreboard re-sync that the code does not do | `vendor/cs2-kit/src/Sdk/PlayerController.cpp:246-262 (memset+memcpy only, no state-change call) vendor/cs2-kit/docs/sdk.md:113 ("issues NetworkStateChanged so the scoreboard re-syncs")` |
| SDK-9 | L | c | Y | duplicate | EntitySystem duplicates entity-system resolution logic in Initialize() and GetEntitySystem() | `vendor/cs2-kit/src/Sdk/Entity.cpp:54-58 (Initialize resolves EntitySystem) vendor/cs2-kit/src/Sdk/Entity.cpp:72-80 (GetEntitySystem re-resolves with identical reinterpret_cast)` |

### Punishments / Chat / CheatCheck / Web

| ID | SEV | V | LIB | KIND | Finding | Key locations |
|----|-----|---|-----|------|---------|---------------|
| SUP-2 | H | c |  | duplicate | VoiceMuteRepository and TextMuteRepository are byte-for-byte twins | `src/Database/Repositories/VoiceMuteRepository.cpp:1-142 src/Database/Repositories/TextMuteRepository.cpp:1-141` |
| SUP-3 | H | c | Y | boilerplate | Every repository query repeats the same try/catch-return-default block | `src/Database/Repositories/BanRepository.cpp:13-174 src/Database/Repositories/VoiceMuteRepository.cpp:13-118` |
| SUP-4 | H | c | Y | structure | HttpClient is a generic async-HTTP component stranded in the plugin | `src/Web/HttpClient.hpp:1-71 src/Web/HttpClient.cpp:1-149` |
| SUP-1 | M | p |  | duplicate | Issue{Ban,VoiceMute,TextMute,Warning} share one duplicated skeleton | `src/Punishments/PunishmentManager.cpp:150-183 src/Punishments/PunishmentManager.cpp:185-211` |
| SUP-5 | M | p |  | duplicate | Remove* and Remove*BySteamId form three identical method triplets | `src/Punishments/PunishmentManager.cpp:278-335 src/Punishments/PunishmentManager.cpp:337-372` |
| SUP-6 | M | c |  | complexity | Two divergent broadcast APIs; one keys off English string literals | `src/Core/ChatService.cpp:51-73 src/Core/ChatService.cpp:75-94` |
| SUP-7 | M | c |  | duplicate | Mute-notice rendering is duplicated between text and voice paths | `src/Core/ChatService.cpp:145-172 src/Core/ChatService.cpp:184-211` |
| SUP-9 | M | p | Y | structure | EscapeHtml and the center-HTML banner/panel scaffolding are reusable, not CheatCheck-specific | `src/Admin/CheatCheck/CheatCheckView.cpp:49-78 src/Admin/CheatCheck/CheatCheckView.cpp:111-130` |
| SUP-10 | L | c |  | churn | Dead teamOnly branch in RebroadcastAdminChat (both arms identical) | `src/Core/ChatService.cpp:115-125` |
| SUP-11 | L | c |  | boilerplate | ReplyToAdmin callers pass a lambda only to wrap a one-line format | `src/Admin/CheatCheck/CheatCheckManager.cpp:167-169 src/Admin/CheatCheck/CheatCheckManager.cpp:174-181` |
| SUP-12 | L | p |  | duplicate | Cancel/Expire/CancelAll repeat the restore-state-then-teardown dance | `src/Admin/CheatCheck/CheatCheckManager.cpp:250-265 src/Admin/CheatCheck/CheatCheckManager.cpp:267-285` |
| SUP-8 | L | p |  | duplicate | CheatCheck View duplicates its PanelState switch in Render and RenderPanel | `src/Admin/CheatCheck/CheatCheckView.cpp:82-131 src/Admin/CheatCheck/CheatCheckView.cpp:133-157` |
