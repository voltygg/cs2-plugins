# Stronghold — build-and-defend team deathmatch

Status: planned 2026-09-20; implementation started 2026-09-21 on `feat/stronghold` (voltmod, plugin submodule, root). Name **Stronghold** (plugin `plugins/stronghold/`, log tag `Stronghold`, command prefix `sh_`, chat `!shop`). Implement milestone by milestone; every framework change is a commit pair (voltmod first, then this repo with the relocked `conan.lock`).

## 1. What it is

Team deathmatch with instant respawn and an economy. Kills pay money; money buys **structures** you place in the world (sentry turret, laser mine, wall, ...), **perks** for yourself, and later **vehicles** (drones, tank). Structures level up 1→3, have health and can be destroyed for a reward. Unlike the inspiration, the fight has a goal: each team defends a **Core**, and the round ends when one falls (section 2a lists everything that is ours rather than borrowed). The reference mode's screenshots and decompiled asset sources are in `references/wwdm/` (read-only).

Decisions already made:

- All game logic is server-side in one plugin. Models, particles and sounds ship in a separate `stronghold` workshop addon (id in config, 0 skips it); Panorama screens stay in `meatgg_ui`. A second addon costs a Stronghold player one extra reconnect on first join (`voltmod/docs/workshop.md`), but keeps ~100 MB of models off every other server and lets assets and UI update independently.
- The shop, placement prompt and look-at panel use framework menus and center text first; custom Panorama screens come later if needed.
- Flat source layout, one file per concept, files kept under ~300 lines. One `Structure` struct with a kind switch, stats from config.
- No foreign asset or author names in anything shipped or committed outside `references/`.
- v1 has no database: money, perks and structures live for the map. VIP tiers (M4) add persistence.
- Framework gaps are closed in voltmod as general-purpose APIs (damage, trace), never as plugin-side signature hacks — plugins cannot bind signatures (`voltmod/docs/sdk/gamedata.md`).
- Vehicles are last and gated on a camera prototype. The mode must be fun without them.

## 2. Reference design (from the screenshots)

Economy: $800 per kill; bonus to everyone who upgraded a structure for each frag it makes; reward for destroying enemy structures; start balance $2000. Game's own cash awards are off; balance is `m_iAccount` so the stock HUD shows it.

Shop, three tabs, opened with a key, mouse cursor, `B`/`X` closes:

| Tab | Content |
| --- | --- |
| Loadout | Free weapon sets (AK-47 / M4A4 / M4A1-S / AWP, each + Deagle), armor+helmet $650, HE $300, flash $200, smoke $300, molotov $400, and the equipment grid below |
| Shop | Perks and simple objects |
| VIP | Timed freebies and tier description |

Equipment (price, base limit → limit with VIP tier):

| Item | Price | Limit | Behaviour |
| --- | --- | --- | --- |
| Laser mine | 1000 | 2 → 3 | Beam between mine and opposite surface; enemy crossing dies; allies pass |
| Turret | 2000 | 2 → 3 | Auto-aims and fires at enemies in line of sight; 3 levels (separate head models) |
| Air defense | 1000 | 2 → 3 | Shoots down enemy drones and artillery rockets |
| Health dispenser | 1200 | 1 → 2 | Heals allies nearby |
| Money dispenser | 2000 | 1 → 2 | Pays allies nearby |
| Tesla coil | 2000 | 1 → 2 | Repairs nearby structures, destroys enemy grenades in range |
| Rocket battery | 5000 | 1 | One-shot salvo on a chosen area; must be placed under open sky; map-wide "rocket warning" |
| Wall | 1500 | 2 → 3 | 1000 HP bullet-blocking cover |
| Net | 1200 | 1 → 2 | Overhead net that catches enemy drones |
| Landmine | 800 | 1 → 2 | Explodes when stepped on; breaks when shot |
| Scout drone | 3500 | 1 | Flyable, recon + kamikaze strike |
| Gun drone | 2000 | 1 | Flyable, fires from the air |
| Tank | 8000 | 1 | Drivable, heavy armor, cannon |

Perks: speed +10 %/level ×3 ($2000), gravity −10 %/level ×3 ($2000), +25 HP up to 300 until death ($350), income +10 %/level ×3 ($2500), regen +1 HP/s per level ×3 up to 100 HP ($1500).

Placement: buying puts you in placement mode — translucent ghost in front of you, green when valid / red when not, prompt "Can place · $1000 — [E] Place [R] Cancel". Money is charged on placement, not on entering the mode.

Look-at panel: aiming at a structure shows owner, health, level ("1 of 3 (upgradable)"), state, frags, and "[E] Upgrade to level 2 — $1200". Anyone on the team may pay for the upgrade.

First join: a three-card "How to play" screen with an "I understand" button.

## 2a. What makes it ours

The reference is an endless deathmatch where building is a side activity. Stronghold gives building a purpose and adds counterplay. All of these are cheap on top of the M2 core (distance checks, timers, one more prop); none needs new framework work beyond section 5. Each has a config switch so a server can run the plain variant.

1. **Team Cores and rounds (headline).** Each team has a Core — a large high-health structure near its spawn. Destroying the enemy Core wins the round; if the timer runs out, the Core with more health wins. Cores only take damage from players and structures within a radius (no cross-map AWP chipping) and announce "Core under attack". Between rounds: short scoreboard with top builder / top destroyer, then bases are wiped and money partly carries over (config, default 50 %). Core positions per map come from a small config file with an admin command to set them in game (`sh_core_set <team>`), falling back to the team's spawn centroid.
2. **Bases go offline, not away.** While the owner is dead their structures are *unpowered* (no firing, no healing, dimmed) and come back on respawn. They are removed only on disconnect or team change. This replaces the reference rule "structures die with the owner", which makes building pointless in an objective mode.
3. **Sabotage.** Hold E for 3 s on an enemy structure from behind: it switches sides for 30 s, then self-destructs. The owner gets a warning toast and can cancel it by hitting the saboteur. Turrets do not see a crouching enemy outside their front cone, so flanking is a real tactic.
4. **Different toolbox.** Before any vehicle work, three cheap structures the reference does not have:
   - **Sensor tower** — enemies within range glow through walls for the owner's team (the framework's per-viewer glow, `Hooks/GlowVision.hpp`).
   - **Teleporter pair** — entrance and exit, recharge time shortens with level.
   - **Jump pad** — launches allies along the direction it was placed in.
5. **Turret specialisation.** Level 3 is a choice, not a stat bump: *Gatling* (fast, short range), *Marksman* (slow, long range, high damage), *Cryo* (low damage, slows targets). Shown as three buttons in the look-at panel.
6. **Scrap and bounties.** A destroyed structure drops a scrap pickup worth part of its price — anyone can grab it, so wrecking a base near the enemy is risky. A player on a kill streak or with a large base carries a bounty shown on the scoreboard tag; the team that is behind on Core health earns a small income boost.
7. **Supply drops.** Every few minutes a crate lands at a contested point (map config or the midpoint between Cores) with a beacon and a siren; first to hold E on it gets a free random level-2 item.
8. **Fair VIP.** Tiers give cosmetics (structure skins, beam colors, kill-feed tag), convenience (loadout presets, faster placement) and a modest salary — not higher structure limits. The reference sells power; this is an owner decision, see section 11.

Deliberately dropped from the reference unless M5 happens: tank, both drones, rocket battery, air defense, net. They are the most expensive items and the least original.

## 3. Ground truth

### Framework, ready today

- Spawn/remove/keyvalues/inputs/model: `voltmod/include/VoltMod/Entities/EntityOps.hpp`, `KeyValues.hpp`, `Entity.hpp` (`Teleport`). Worked `prop_dynamic` example incl. `FollowEntity` parenting: `voltmod/src/Hooks/GlowVision.cpp:37-77`.
- Per-player visibility for the ghost: `Hooks/Visibility.hpp` (`ShowOnlyTo(EntityRef, slot)`); alpha/color: `Entities/Render.hpp` (`SetRender`, `RenderMode_t::TransTexture`).
- Line traces: `Entities/Trace.hpp` (`Line`, `Clear`, `TraceOptions{Layers, Ignore1, Ignore2}`) — result has `Hit`, `Fraction`, `End` only.
- Frame loop and timers: `Core/Time/Scheduler.hpp` (`EveryFrame`, `Repeat`, `Delay`); registrations are `Subscription`s.
- Input: `runtime.Hooks.Movement.Before/After` with `Hooks/PlayerInput.hpp` (`ButtonsHeld`, `ButtonsChanged`, view angles); `EntitySystem::Buttons(slot)`; `IN_USE = 0x20`, `IN_RELOAD = 0x2000`. **Read-only** — `Rewrite` edits only the decoded copy (`src/Hooks/Movement.cpp:43-46`).
- Player state: `Controller::Money/SetMoney`, `Pawn::SetHealth/SetArmor/SetVelocity/SetMove/SetSpeedModifier` (the speed modifier decays, it is not a setting).
- UI: `Ui/ScreenManager.hpp` (`ForPlayer`, `Shared`, `Pressed` → `ButtonPress{Slot, ButtonId}`), `Ui/Screen.hpp` (`SetText`, `SetClass`, `SetHidden`, `ShowCursor`), blocks in `voltmod/panorama/blocks/` (`tabs`, `card`, `button`, `bar`, `toast`). Docs: `voltmod/docs/custom-ui.md`, `panorama.md`. Reference wiring: `plugins/main-menu/src/App.cpp`. Limits: only Panel/Label/Image/Button; 400 interned names per screen, 1024 global (`voltmod panorama check`); never build ids at runtime.
- Main-menu entry: publish `Contracts::IMenuSection` (`plugins/contracts/include/Contracts/IMenuSection.hpp`).
- Sound: `EntityOps::EmitSound` / `EmitSoundFilter` take a soundevent name. Precache: `runtime.World.Precache.Add(path)` — takes effect on the **next map load**.
- Addon requirement: `Workshop/Addons.hpp` (`Require(id)`).
- Events: `Events/EventTypes.hpp` (`player_death`, `player_spawn`, `player_hurt`, `round_start`, ...); forging events: `GameEvents::CreateEvent/FireEvent`.
- Config: `VoltMod::Options<Settings>` over `configs/settings.jsonc`. Commands: `Commands/CommandBuilder.hpp`. DB (M4 only): `Database/Api.hpp`, `docs/database.md`.

### Framework, missing (section 5)

1. No damage hook, no apply-damage call → no kill credit for turrets, no structure health from bullets.
2. `TraceHit` has no hit entity and no surface normal; no hull trace.
3. `m_flGravityScale`, `m_flMaxspeed`, `m_bTakesDamage`, `m_iMaxHealth` are in `voltmod/schema/server.windows.json` but not in `schema/manifest.json`.
4. No camera/view override and no usercmd writeback (vehicles only).
5. Particles and beams: spawnable through `EntityOps::Spawn` (`info_particle_system` + `effect_name`, `env_beam`) but unproven here.

Signatures and layouts to port: `references/CS2Fixes/gamedata/cs2fixes.jsonc` (`CBaseEntity_TakeDamageOld` ≈:99, `CTakeDamageInfo` offsets ≈:225, `CCSPlayerPawn::OnTakeDamage_Alive` ≈:437); field layouts in `references/swiftlys2/generator/datamaps.json`. Re-verify every signature against the current `server.dll`/`libserver.so` — see memory "Verifying vtable indices".

### Reference assets (`references/wwdm/`, decompiled source)

- Models (ModelDoc `.vmdl` + `.dmx`): turret `stand`, `stand_high`, `level1..3` (levels 2/3 skinned with a `spin` animation, material groups `blue`/`red`), laser (`laser_emitter` attachment), wall (PhysicsMesh), landmine, net, pole, tesla, dispenser (groups `ct_1..3`/`t_1..3`), money dispenser (`level_1..3`), air defense and rocket battery parts, tank parts (turret has a `gun` attachment), drones. All have physics hulls except the zone wall (skipped).
- Turrets have **no muzzle attachment**: the muzzle is a per-level offset in config. There is **no beam particle**: the laser uses `env_beam`.
- Particles: tesla arcs/zaps per team and level, mine beacons, dispenser transfer/break, drone muzzle/shock, rocket exhaust.
- Soundevents: buildables (building, start, detect, upgrade, laser deploy/charge/activate/beam), turret shot, tank, air (rocket battery, air defense).
- Panorama layouts are obfuscated; reference only.

## 4. Milestone 0 — re-home the reference assets

- [x] Create the `stronghold` addon in the Workshop Tools (`<CS2>/content/csgo_addons/stronghold/`).
- [x] Copy the needed sources from `references/wwdm/` under our own paths: `models/stronghold/...`, `materials/stronghold/...`, `particles/stronghold/...`, `sounds/stronghold/...`, `soundevents/soundevents_stronghold.vsndevts` with names like `Stronghold.Build.Upgrade`. Rename every file and internal reference that carries a foreign prefix.
- [x] Compile with the local `resourcecompiler`; fix errors; leave out and note anything that still fails.
- [x] Case-insensitive grep over the addon tree and `plugins/stronghold` for the foreign prefixes comes back empty.
- [x] `plugins/stronghold/docs/assets.md`: path, source ("reference pack" or own), licence status.

Ownership: the reference assets belong to their original authors. Treat them as development stand-ins; the `assets.md` table is the list to replace or clear before the addon is published publicly. The item list is config-driven, so swapping a model is a one-line change.

## 5. Milestone 1 — framework work (voltmod)

Each item: gamedata entry + `Bindings` member + public API + doc page + doctest where SDK-free. Windows and Linux signatures both.

### F1 Damage

- [x] Bind `CBaseEntity::TakeDamageOld` and describe `CTakeDamageInfo` (attacker, inflictor, ability handles; damage; damage type; hit group; position). Accessor struct, no raw offsets in plugins.
- [x] `runtime.Hooks.Damage.Before(handler)`: handler sees victim entity + mutable damage info and returns allow / block. Fires for **every** entity, not just players.
- [x] `Damage::Apply(victim, spec)` with `spec{Attacker, Inflictor, Amount, Type}`: builds a `CTakeDamageInfo` and calls the engine, so death, kill feed, `player_death` and stats are the engine's own.
- [x] Spike before polishing: one `prop_dynamic` that (a) kills a bot through `Apply` with the owner credited, (b) reports bullet damage through `Before`. If bullets never reach `TakeDamageOld` for `prop_dynamic`, try `m_bTakesDamage`/`m_takedamage`, then `prop_physics_override` with motion disabled. Record the winning recipe in the doc page.

### F2 Trace

- [x] Add `HitEntity` (as `EntityRef`/handle) and `Normal` to `TraceHit`.
- [x] Add a hull trace (`Trace::Hull(from, to, mins, maxs, options)`). If the nav trace cannot sweep a box, bind the physics query path CS2Fixes/SwiftlyS2 use (`TraceShape`).

### F3 Schema manifest

- [x] Add `m_flGravityScale`, `m_flMaxspeed` (movement services), `m_bTakesDamage`, `m_iMaxHealth`, and whatever F1's spike needs; `voltmod schemagen`; expose `Pawn::SetGravityScale`, `Pawn::SetMaxSpeed`, `Entity::SetMaxHealth`.
- [x] Confirm the speed perk survives weapon switches (the engine recomputes max speed per weapon). If it does not, reapply in `Movement.After`.

### F4 Effects helpers (small)

- [ ] Prove `info_particle_system` (`effect_name`, `start_active`, `Start`/`Stop`/`DestroyImmediately`) and `env_beam` or a two-control-point particle for the laser. If a thin `Effects::PlayParticle(path, origin, angles, lifetime)` falls out, add it to voltmod; otherwise keep it in the plugin.

### F5 Vehicles prerequisites (do in M5, listed here for completeness)

- [x] Camera: prototype in order — (1) observer mode targeting the vehicle entity (`Pawn::SetObserverMode`, `CPlayer_ObserverServices::SetObserverTarget`), (2) `point_viewcontrol`-style camera entity if CS2 still honours it, (3) hiding the pawn and moving it as the vehicle. Pick the first that gives a stable first/third-person view with the player's pawn safe and parked.
- [x] Usercmd writeback or a "suppress movement" flag in the Movement hook, if `MoveType::None` + reading input proves insufficient.

Release: tag voltmod, `uv run poe build --relock` from this repo's root, commit `conan.lock` (memory "Shipping a framework fix to prod").

## 6. Milestone 2 — plugin core (playable with turret, mine, wall)

Scaffold: `uv run poe new-plugin stronghold`; `plugin.json` depends on main-menu contracts only. Suggested source layout (plain names, one responsibility each):

```text
plugins/stronghold/
  configs/settings.jsonc      item table, prices, limits, level stats, rewards, perk steps
  configs/stronghold.cfg      game convars for the mode
  panorama/screens/           shop, hud, tutorial (.xml.j2 / .css.j2)
  src/App.cpp                 wiring only
  src/Economy/                Wallet (balance, rewards, income perk)
  src/Shop/                   ShopScreen (tabs, cards, purchase results), Loadouts
  src/Placement/              PlacementMode (ghost, validity, confirm/cancel)
  src/Structures/             Structure (data), StructureRegistry (owner/limits/lookup/cleanup),
                              Turret, LaserMine, Wall, Landmine, Dispenser, ...
  src/Perks/                  PlayerPerks (apply on spawn, reset rules)
  src/Hud/                    LookAtPanel, prompts, toasts
  tests/                      doctest, SDK-free cores
```

Rules of the road: hold entities as `EntityRef`, never raw pointers across frames; every `Subscription` is a member declared after what it captures; item stats come from config, not constants; follow `.claude/rules/cpp.md` and `framework-patterns.md`.

- [x] **Mode rules** — `stronghold.cfg`: instant respawn for both teams, no buy zone/time, game cash awards off, max money raised, no round end on elimination, long round timer, warmup off. Apply on map start.
- [x] **Wallet** — start balance, kill reward (income perk multiplier), structure-destroy reward, upgrader bonus per structure frag, `CanAfford`/`Charge`/`Pay`. Mirrors into `Controller::SetMoney`. Pure logic unit-tested.
- [x] **Config** — `Settings` with an item list: id, kind, title key, price, base limit, per-level `{health, damage, range, fireInterval, upgradePrice}`, model paths per part, sound names. Reloadable.
- [x] **Shop screen** (done as a framework menu per the review; no Panorama screen and no key bind yet) — one `ForPlayer` screen, three tab panels toggled by class, static card ids (`shop_item_turret_buy` ...). Per card: price, owned/limit, state class (`can-buy` / `no-money` / `limit` / `need-vip`). Balance label. Opens on `!shop`, `sh_shop`, main-menu section, and a key: test whether the client sends `drop` (G) or `buymenu` (B) as hookable commands in this mode; bind whichever works and document the fallback `bind`. `ShowCursor` on open, off on close/death/disconnect. Keep the name count under the budget; run `uv run poe lint`.
- [x] **Loadouts** — free weapon set applied now and on each spawn; armor and grenades as one-off purchases.
- [x] **Placement mode** — enter on purchase click (shop closes). Ghost = same model parts, `ShowOnlyTo` the placer, translucent green/red via `SetRender`. Each frame: trace from eye along view to max distance, snap to hit point, orient by surface normal (floor items upright and yaw = player yaw; laser mine and wall-mounted items align to the wall normal). Validity: surface slope per item, hull trace clear of world/players/structures, minimum distance from spawn points and other structures, item-specific checks. E places and charges; R, weapon fire, death or shop reopen cancels. Validity rules unit-tested on plain vectors.
- [x] **StructureRegistry** — owns all live structures: owner slot + SteamID, team, kind, level, health, frag count, list of upgraders, entity refs for parts, powered flag. Enforces per-owner limits. Owner death → unpowered until respawn (dim via `SetRender`, behaviours skip unpowered structures); removed on disconnect, team change, round/map end. Lookup by `EntityRef` for the damage hook and look-at.
- [x] **Cores and round flow** — spawn one Core per team at the configured position, large health, damage accepted only from attackers within the radius, "under attack" alert with a cooldown, win on destruction or by health at timeout, end-of-round summary screen, wipe structures, carry over money by the configured share. `sh_core_set <team>` writes the map's position file. Round outcome logic unit-tested.
- [x] **Structure health** — in `Damage.Before`: victim is a registered part → subtract from the structure's health (friendly fire ignored), block engine damage, pay the destroyer at zero, play break effect, remove. Hit feedback sound.
- [x] **Turret** — states: building (short delay) → idle sweep → tracking → firing. Target selection at ~10 Hz, staggered across turrets: nearest living enemy in range with `Trace.Clear` from the muzzle, ignoring own parts. Yaw part and pitch part rotate toward the target at a capped turn rate every frame. Fires on an interval through `Damage::Apply` (attacker = owner pawn, inflictor = turret), tracer/muzzle particle, sound. Levels swap the head model and stats. Targeting math (lead-free aim angles, turn-rate clamp, range/FOV test) unit-tested.
- [x] **Laser mine** — placed on a wall; beam endpoint from a trace along the normal; each tick test enemies against the segment (closest-point distance, cheap) and kill through `Damage::Apply`. Team-colored beam. Breaks when shot.
- [x] **Wall** — static solid prop with health. Nothing else.
- [x] **Look-at panel and upgrade** — per player at ~10 Hz: trace with `HitEntity`, resolve to a structure, fill the HUD panel (`SetText`/`SetHidden`). E edge while looking at an allied upgradable structure within reach charges the presser, levels it up, records them as an upgrader. "Not enough money: need $1200" toast.
- [ ] **HUD** — shared-style HUD done as a per-player screen: prompts, toasts ("+$800"), rocket warning banner slot for later.
- [ ] **Tutorial** — first-join screen with three cards and a confirm button; remembered for the map in v1.
- [x] **Precache and addon** — `Precache.Add` for every configured model/particle/soundevent file at load; `Addons.Require(3801580041)`. Center-text fallback while a client is still downloading.
- [x] **Translations** — English and Russian for every player-facing string.
- [x] **Other plugins** — make sure `anticheat` does not flag speed/gravity perks or turret kills (no attacker view angles), and that `bhop` is not loaded on these servers or tolerates modified max speed.

Exit criteria: on a local server with bots, buy → place → turret kills with kill-feed credit → enemy destroys it for a reward → owner death powers the base down and respawn brings it back → a destroyed Core ends the round; 32 bots with 60+ structures holds tick rate (profile the targeting loop).

## 7. Milestone 3 — signature features, remaining structures, perks

Signature features first (section 2a) — they are what players will not find elsewhere:

- [x] Sabotage: hold-E progress bar on the HUD, behind-the-structure test (dot product against its facing), temporary team flip, self-destruct timer, owner warning, cancel on damage to the saboteur. Turret front-cone rule for crouching enemies.
- [x] Sensor tower: per-team glow of enemies in range using the `GlowVision` recipe; range grows with level.
- [x] Teleporter pair: two placements in one purchase, recharge per level, telefrag protection (exit must be clear — hull trace), team-colored particle.
- [x] Jump pad: launch velocity along placement yaw, per-player cooldown, no fall damage for the landing.
- [x] Turret specialisation at level 3 (Gatling / Marksman / Cryo) — stats per branch in config; Cryo slow via the max-speed setter with a timed restore.
- [x] Scrap pickups (prop + proximity check + despawn timer), bounty tracking and payout, trailing-team income boost.
- [x] Supply drops: schedule, drop point choice, falling crate, beacon particle and siren, hold-E claim, random level-2 item placed into the claimer's placement mode for free.

Then the borrowed basics:

- [x] Landmine (proximity trigger by distance check, explosion via `env_explosion` or radius `Damage::Apply`, breaks when shot).
- [x] Health dispenser and money dispenser (aura tick, transfer particle, level screens via material group/skin).
- [x] Tesla coil (repairs allied structures in range; removes enemy grenade projectiles in range — find by classname each tick; arc particles).
- [x] Perks: speed, gravity, +25 HP (until death), income, regen. Decide and document which perks reset on death.
- [ ] Balance pass on a live test server; all numbers in config.

## 8. Milestone 4 — VIP tiers and persistence

- [x] Tiers `basic / lite / medium / ultra / extreme` as permission strings (`stronghold.vip.<tier>`) resolved through `runtime.Policy` so admin-system groups can grant them. No new tables unless expiry dates are needed; if they are, a `stronghold` DB with its own migrations (test data in seed files, not migrations).
- [x] Tier effects from config. Default set is the fair one from section 2a: structure skins, beam colors, kill-feed tag, loadout presets, faster placement, modest salary. The reference's power perks (higher limits, timed free HP/armor/money, discounts) stay available as config options, off by default.
- [x] VIP tab: shows current tier, cooldown timers, and what each tier adds. Movement extras (extra air jumps, parachute, grapple) only if wanted — each is its own small task.
- [ ] Optional persistence of lifetime stats (structures built, structure frags) for a leaderboard.

## 9. Milestone 5 — air and armor (optional)

Optional and last: this is the part copied most directly from the reference and the most expensive. Gate: players ask for it **and** the F5 camera prototype works. Otherwise M2–M4 are the product.

- [x] Rocket battery: sky check (upward trace must reach sky/no hit), target picked by looking at a ground point, warning banner + siren to everyone, salvo of rocket props moved per frame on ballistic arcs with exhaust particles, radius damage on impact, then the battery removes itself.
- [x] Air defense: targets enemy rockets and drones in range, fires interceptor props, destroys the target on proximity.
- [x] Net: overhead solid that drones collide with.
- [x] Scout drone and gun drone: pilot's pawn parked and protected or vulnerable (decide), input read from `PlayerInput`, drone moved by velocity with simple collision traces, battery/lifetime, markers on spotted enemies, strike / gun fire through `Damage::Apply`. Exit on E or destruction.
- [x] Tank: hull + turret + gun + tracks as parented parts, ground-following by downward traces, turret follows view yaw, cannon shell as a moved prop with radius damage, heavy health, crush damage optional.

## 10. Testing and rollout

- Unit tests (doctest, SDK-free): wallet, limits, placement validity, targeting math, segment-vs-player test, perk stacking.
- Live: `/build-local` then `uv run poe build --install stronghold --start`; bots as targets; `/rcon-debug` for convars and commands. Check on a real client: shop clicks, ghost visibility only to the placer, kill feed, spectator view of another player's HUD.
- Linux: every new signature verified on `libserver.so` before the first `/deploy-test`.
- Rollout: one test server, addon update pushed early (workshop moderation delay — memory "Workshop addon client cache"), then more servers.

## 11. Open questions for the owner

1. VIP: fair (cosmetics and convenience, the plan's default) or power perks like the reference? This is a revenue-versus-reputation call.
2. Asset plan after development: commission replacements, buy packs, or get permission for the extracted ones?
3. Maps: stock competitive maps only (as the reference does), or a curated pool with blocked placement zones? Cores need a sensible position per map either way.
4. Which of the section 2a features to keep — all have a config switch, but each one kept is work in M3.

## Progress

### 2026-09-21 — plan review

- Corrections applied: `voltmod/` paths, separate `stronghold` addon, asset facts, menu-first UI, flat layout, file-size limit, no foreign names, README in the plugin repo.
- Framework recipe from `references/`: damage via a `CBaseEntity::TakeDamageOld` detour plus the `CTakeDamageInfo` constructor (CS2Fixes, SwiftlyS2); trace hit entity, normal and hull via `CNavPhysicsInterface::Nav_TraceShape` (CS2AC); camera via `CPlayer_CameraServices::m_hViewEntity` (CS2Fixes).

### 2026-09-21 — Milestone 0, assets

- Landed: the `stronghold` addon. Sources are in `<CS2>/content/csgo_addons/stronghold/` and compiled files in `<CS2>/game/csgo_addons/stronghold/` (with `addoninfo.txt`). `<CS2>` is `C:/Program Files (x86)/Steam/steamapps/common/Counter-Strike Global Offensive`. Stronghold commit `684c136` adds `docs/assets.md`, which lists the assets and the model, particle and soundevent paths the plugin uses.
- Compiled with no failures: 32 models, 41 materials, 57 textures, 43 particles, 28 sounds and one soundevents file. Nothing was left out. The particle textures under `materials/stronghold/fx/` don't compile as particle children, so they need their own `resourcecompiler` pass on the `.vtex` files.
- Renames: `pvo_v2` became `air_defense`, `grad_v2` became `rocket_battery`, and `lfence` became `laser`. The `wwdm/` models moved up to `models/stronghold/<name>/`. Tracers are now `particles/stronghold/tracers/`, and the laser soft trail is `tracers/laser_trail.vpcf`. The dispenser sounds are `dispenser_use`, `dispenser_destroy` and `dispenser_deny`. The binary DMX string tables were rewritten in place. VRF's stale `Compiled Textures` blocks were removed from the `.vmat` files.
- Deviations: model materials stay next to their models, as they were in the pack. `materials/stronghold/` only holds the particle textures and the shared roughness map. The 11 buildables, 1 turret, 13 air, 7 tank and 9 drone events are merged into a single `soundevents_stronghold.vsndevts`. Drone events use the pack's spatial `.pos` variants. Some events and particles still point at stock CS2 sounds and particle textures, which ship with the game.
- Skipped on purpose: zone wall, chest, gift, drone kit, the `fx_*` and `zone_*` particles, and `dota_leaves`.
- Check: a case-insensitive grep for `cs2red|wwdm|warsdm|letaryat|icsdm` over both addon trees and `plugins/stronghold` comes back empty. It covers file names and binary content.
- User action: upload the addon in the Workshop Manager, then set its id as the plugin's `addonId` config value. 0 skips `Addons.Require` for local runs. The plugin must precache `soundevents/soundevents_stronghold.vsndevts`. No model has been viewed in-game or in ModelDoc yet, so check the scale, the material groups and the tank attachments on first spawn.

### 2026-09-21 — Milestone 1, framework APIs (step 1)

- Landed in voltmod on `feat/stronghold` (not pushed, not tagged, `conan.lock` not relocked; the root keeps `conan editable add voltmod` so the next steps build against the checkout):
  - `834851e` feat: hook and apply entity damage through CBaseEntity::TakeDamageOld
  - `5187d3b` feat: report the hit entity and normal and sweep hull traces
  - `1c7191e` feat: generate max health, damage intake, gravity, max speed and camera fields
  - `b1c7ac0` feat: add AngleToForward for turning an aim into a direction
  - `72a1606` fix: give applied damage a position and a push away from the inflictor
  - `1f62239` docs: record the prop damage recipe and the hull fit check
- API (from `<VoltMod/Hooks/Api.hpp>` and `<VoltMod/Entities/Api.hpp>`):
  - `runtime.Hooks.Damage.Before` is `Event<DamageHit&>`, where `DamageHit{Entity Victim; DamageInfo Info; bool Blocked}` and `DamageInfo{EntityRef Attacker, Inflictor; float Amount; uint32_t Type}`. Set `hit.Blocked = true` to cancel. Edits to `Amount` and `Type` reach the engine; `Attacker` and `Inflictor` are read-only there.
  - `Status runtime.Hooks.Damage.Apply(const Entity& victim, const DamageInfo& info) const`. An empty `Inflictor` falls back to the attacker. The hit lands at the victim's origin, pushed away from the inflictor. Damage type bits are `VoltMod::DamageBullet`, `DamageBlast` and the rest.
  - `TraceHit` gained `Normal` and `HitEntity` (an `EntityRef`; the world is `worldent`, index 0). New: `Result<TraceHit> runtime.World.Trace.Hull(from, to, mins, maxs, options = {})`.
  - Schema: `Entity::MaxHealth/SetMaxHealth`, `TakesDamage/SetTakesDamage` and `GravityScale/SetGravityScale` (these are on CBaseEntity, so every wrapper has them), `pawn.MovementServices().MaxSpeed/SetMaxSpeed`, `pawn.CameraServices().ViewEntity/SetViewEntity`, and `Schema::CCSPlayerBase_CameraServices{pawn.CameraServices().Base()}` with `FieldOfView` and `ZoomOwner` (for F5).
  - `Vector VoltMod::AngleToForward(const QAngle&)` in `<VoltMod/Entities/Angles.hpp>`.
- Deviations:
  - A `Before` handler cannot return a value, because `Event` handlers return void. So it sets `DamageHit::Blocked` instead of returning false.
  - Damage is a service method, `runtime.Hooks.Damage.Apply(...)`, not a free `Damage::Apply`.
  - Max speed is reached through the generated view (`pawn.MovementServices().SetMaxSpeed`), not a hand-written `Pawn::SetMaxSpeed`. That is the same one hop the docs use for the money services.
  - The camera fields were added now rather than in step 7. The F4 particle proof stays plugin-side, as the review said, so its box is still open.
  - `AngleToForward` has no doctest, because it takes SDK types and voltmod tests stay SDK-free. The damage binding got a doctest line in `BindingsTests`.
- Verified on Windows, server build 2000908:
  - `voltmod gamedata check`: 23/23 patterns hold. That includes `CBaseEntity::TakeDamageOld` (rva 0x3E5660) and `CTakeDamageInfo::CTakeDamageInfo` (rva 0xEA5600).
  - `CNavPhysicsInterface::Nav_TraceShape` is at Windows index 3. The offline vtable dump puts slot 3 at `server+0xF876E0`: the thunk that swaps a default filter in for a null argument and tail-jumps. `resolved.windows.json` on the live server records the same `code` 16283360 (0xF876E0).
  - The host logged "Schema: the generated layout matches game build 2000908" with the new fields.
  - `CGameTrace` and `Ray_t` come from the SDK headers, not from hand offsets. Both trace buffers are `alignas(16)` for Linux.
- Live spike (local server, de_dust2, 6 bots):
  - The damage hook fired for players (victim `player`, attacker and inflictor the shooter's pawn, type 0x2). It also fired for bullets hitting the world, with victim `worldent#0`.
  - `Apply(bot5, {Attacker = bot4, 500, DamageBullet})` killed the bot, and `player_death` reported `victim=5 attacker=4 weapon=hkp2000`. The weapon is the attacker's active weapon, so a turret kill shows the owner's current gun in the feed.
  - Before the fix commit, the engine warned "damagetype 2 with info.GetDamageForce() == Vector::vZero". The fix removed the warning.
  - Traces: looking at a wall gave `normal=(0,-1,0) entity=worldent#0`. Looking down at the floor gave `normal=(0.01,0.01,1.00)`. A hull stops a little short of the line. A zero-length hull at a pawn with no ignore reports start-solid and `HitEntity=player`. A line through a spawned crate reports `entity=prop_dynamic#179`.
  - Max speed: `SetMaxSpeed(400)` on a bot read back 400 five seconds later. Whether it changes real movement speed, and whether it survives a weapon switch, needs a client.
- Prop damage recipe (also in `voltmod/docs/sdk/hooks.md`):
  - Bullets reach `Damage.Before` for a `prop_dynamic` spawned with a precached model and `solid` 6. No health setup is needed; the victim is the prop and the attacker is the shooter's pawn.
  - The engine keeps no health on `prop_dynamic`: `health` 500, `SetMaxHealth(500)` and `SetTakesDamage(true)` all left it at 500 after hits. Structures must track their own health in `Before` and set `Blocked`.
  - Do not parent a structure with `FollowEntity`. It bone-merges the prop, and bullets pass through it.
  - Alternative: a `prop_physics_override` with `spawnflags` 8 and `health` 500 loses health to bullets and to `Apply` (500 → 400 → 374 → 347 → 0), and the engine removes it at zero.
  - The model must be precached (`World.Precache.Add` at load, active on the next map load). Otherwise the prop spawns with no model and no collision.
- Linux: nothing verified, because there is no `libserver.so` on this machine. Check these before any Linux deploy:
  - the two damage signatures (CS2Fixes' bytes, identical in SwiftlyS2);
  - `Nav_TraceShape` index 5, inferred from the ABI (two destructor slots, declaration order);
  - the Linux schema baseline for `CPlayer_CameraServices` and `CCSPlayerBase_CameraServices`. These were copied from the Windows dump, because every pawn component in the baseline has identical offsets on both platforms. The host's layout check refuses the load if they are wrong.
- Blockers and user checks: none blocking. In game, check that a turret kill through `Apply` shows the owner in the kill feed; this was verified only through `player_death`. Also check that `SetMaxSpeed` and `SetGravityScale` change real movement for a human player.

### 2026-09-21 — Milestone 2 core, part A (step 3)

- Landed in `plugins/stronghold` on `feat/stronghold` (not pushed):
  - `b4c5f1b` feat: apply the stronghold game rules on map and round start
  - `4d7b943` feat: add the item table, economy and feature settings
  - `30f1432` feat: keep player money in the controller balance and pay kill rewards
  - `0426c6f` feat: give the chosen free weapon set on every spawn and sell equipment
  - `653f2a9` feat: track placed structures with owner limits, power state and cleanup
  - `3ad25b9` feat: count structure hit points and pay the destroyer
  - `b854600` feat: place structures from a ghost that follows the player's aim
  - `9e65b34` feat: add the shop menu with loadout, structure and VIP pages
  - `3e5d810` fix: end the warmup from the mode rules
- Nothing changed in voltmod. `conan.lock` is not relocked; the root still builds against `conan editable add voltmod`.
- Files (`src/`): `App`, `Commands`, `Config`, `ModeRules`, `Wallet` + `WalletMath` (SDK-free), `Loadouts`, `Structures`, `StructureHealth`, `Placement` + `PlacementRules` (SDK-free), `Shop`. The largest is `Placement.cpp` at 278 lines. Tests: `tests/WalletTests.cpp`, `tests/PlacementRulesTests.cpp` (13 cases).
- API for part B:
  - `Structures` (`src/Structures.hpp`): `Build(item, ownerSlot, origin, angles)`, `Find(EntityRef part)`, `FindById`, `All()`, `CountOwned`, `SetPowered`, `Remove(id)`, `RemoveOwnedBy`, `RemoveAll`, `Forget`. `Structure` has `Id, ItemId, Kind, Owner (PlayerRef), Team, Level, Health, Frags, Upgraders, Parts, Powered, Origin, Angles`. Pointers are valid until the next add or remove; keep `Id` across frames. Every part spawns at the placement point; the turret head offset is part B's.
  - `StructureHealth` blocks every hit on a part, ignores the owner's team, pays `economy.destroyReward` at zero and removes the structure on the next frame (outside the engine's damage call).
  - `Placement::Begin(slot, item)`, `Cancel`, `IsPlacing`. `PlacementRules`: `SurfaceFits`, `FarFromAll`, `YawOf`, `SlopeLift`.
  - Config: `ItemSettings{id, kind, price, limit, models[], levels[{health, damage, range, fireIntervalMs, upgradePrice, model, muzzleOffset}], placement{surface, maxSlopeDegrees, mins, maxs}, sounds{place, upgrade, hit, destroy, fire}}`, `FindItem`, `ParseItemKind`. Also `economy.upgraderBonus`, `features.*` and `addonId`, which are read by nothing yet.
  - `Wallet::Charge/Pay/CanAfford/Balance` work on `Controller::Money`. There is no second copy.
- Deviations:
  - Commit order: loadouts, structures and placement landed before the shop, because the shop wires them together.
  - The wall has no file or commit of its own. It is the generic structure, a solid `prop_dynamic` with tracked health, which is all the plan asks for.
  - `sh_shop` works only as a chat alias (`!sh_shop`). The framework cannot take a player's console command yet. A registered console command is dispatched with no caller. A client command that is not a ConCommand reaches plugins only for `say` and `vote`. A player-console route needs a small voltmod change, or players can `bind <key> "say !shop"`.
  - Precache landed early. `Structures::Precache` adds every configured model plus `soundevents/soundevents_stronghold.vsndevts` at load, because placement cannot be tested without models. `Addons.Require` is still part B.
  - `mp_do_warmup_period` does not exist in CS2, so the cfg runs `mp_warmup_end`. It is a no-op outside warmup (checked live), so running it every round start cannot loop.
  - The cfg runs at load, at map start and on every `round_start`. The load and map-start passes are overridden by the gamemode cfg; the round-start pass is the one that sticks.
  - There is no break particle when a structure is destroyed, only the destroy sound (F4 is still unproven). The sound plays from a part that is removed on the next frame, so it may cut short.
  - main-menu's `settings.jsonc` is unchanged, and it already has 6 tabs. To show the shop in `!menu`, add `{ "kind": "section", "label": "...", "section": "stronghold" }` to a tab.
- Live smoke test (local server, de_dust2, 6 bots, through a temporary console probe that was not committed):
  - Settings parsed and 7 resources were precached.
  - After the first `round_start`, the convars read `mp_respawn_on_death_t 1`, `mp_startmoney 2000`, `mp_maxmoney 60000`, `mp_buytime 0`, `mp_teamcashawards 0` and `mp_roundtime 60`.
  - The kill reward was paid (bot money went 10000 → 10800).
  - `Shop::Open` on a bot: `open=true`, "Menu opened for slot 0 (title: Shop, depth: 1, items: 3)".
  - Placement: the ghost spawned and followed the aim. A bot facing a wall read `TooSteep` for the floor-only wall, and aiming at a player read `NoSurface`.
  - Walls spawned as `prop_dynamic` with 1000 hp. **Real bot bullets hit a wall** (5 hits, 1000 → 856). A same-team `Damage.Apply` was ignored. Three enemy hits of 400 destroyed it, paid the destroyer +500 (10800 → 11300), and the wall was gone on the next frame.
  - Power: killing the owner set `powered=false` and the respawn set it back to true.
  - Cleanup: kicking the owner removed their wall, and `mp_restartgame` removed all structures.
  - Loadouts: every spawn gave the AK-47 and Deagle. Choosing AWP gave it immediately. Armor cost 650 and set armor to 100.
- Local server change: the compiled `stronghold` addon was copied as loose files into `C:/cs2-server/game/csgo/{models,materials,particles,soundevents,sounds}`, because the dedicated server does not mount the addon. None of those folders existed before. Delete them to undo.
- User must check in game:
  - the menu in center HTML (three pages, price rows, balance subtitle);
  - that only the placer sees the ghost, and that it turns green and red;
  - that E places and charges, and that the E press that picks the shop row does not place at once;
  - that R cancels;
  - the placement center text;
  - that a wall is placed facing the player's yaw and at the right scale, and that the hull box (`[-24,-24,0]..[24,24,64]`, a guess) matches the model;
  - that the laser mine faces out of a wall;
  - the dimmed look of an unpowered structure;
  - that the destroy sound plays in full.
  - Bots spent money on their own (for example 2000 → 1000) even with `mp_buytime 0`. Check whether human buying is blocked.
- Blockers: none for part B. The `sh_shop` console route above needs a framework decision.

### 2026-09-21 — Milestone 2 core, part B (step 3)

- Landed (not pushed, not tagged, `conan.lock` not relocked; the root still builds against `conan editable add voltmod`):
  - voltmod `feat/stronghold`:
    - `a1ec6ec` fix!: run a console command typed by a player as that player
    - `cf19765` fix: ignore players in server commands whose handler takes no caller slot
    - `a5d6738` fix: let players type a chat command's console twin, which the engine refused
    - `585f9b0` feat: end the round with a winner through CCSGameRules::TerminateRound
    - `90521ef` feat: generate the beam entity's width and end point
  - stronghold `feat/stronghold`:
    - `34d0265` feat: open the shop from a player's console with sh_shop
    - `34ecbe3` refactor: give the vector type and its engine conversions their own headers
    - `0dddd97` feat: add turrets that track and shoot the nearest visible enemy for their owner
    - `ac1910d` feat: add laser mines whose team-coloured beam kills enemies that cross it
    - `246eaeb` feat: show a structure's panel on aim and upgrade it with E
    - `6d653a8` feat: decide rounds by team Cores with a timer, summary and money carry-over
    - `de643c5` feat: require the stronghold workshop addon when its id is configured
    - `cf4eb42` refactor: share entity removal and the level lookup across structures
  - root: `8262863` feat: add the stronghold shop to the main menu, and this entry.
- Files (`src/`): `Turrets`, `TurretAim` (SDK-free), `StructureAttack`, `LaserMines`, `Segment` (SDK-free), `LookAt`, `Cores`, `RoundFlow`, `RoundRules` (SDK-free), `Vec3` (SDK-free), `Vectors`. The largest are `Placement.cpp` (272) and `Config.hpp` (226); nothing is over 300. Tests: `TurretAimTests`, `SegmentTests`, `RoundRulesTests`; the repo runs 236 CTest cases, all passing. `poe build`, `poe test` and `poe lint` pass.
- API for M3:
  - `StructureAttack::Strike(id, targetPawn, amount, inflictor, damageType)` deals damage through the owner, counts the frag and pays the upgraders. `player_death` runs inside it, so hold ids, not `Structure*`, across the call.
  - `Structures::BuildForTeam(item, team, origin, angles)` builds an unowned structure. `Structure::Effects` holds extra entities removed with it, such as a beam. `RemoveEntities(runtime, refs)`, `LevelAt(item, level)`.
  - `ItemSettings` gained `buildMs`, `scale`, `skins {t, ct}` (the last part's material group, set at spawn), `turret {...}`, `laser {...}` and `sounds.ready`. `Settings.cores {...}`; `building.lookDistance`, `building.useDistance`.
  - `TurretAim`: `AimAt`, `AngleDelta`, `TurnToward`, `InRange`, `OffsetByYaw`, `SweepYaw`. `DistanceToSegment`. `RoundRules`: `DecideRound`, `CarriedMoney`, `RoundStats`, `TopSlot`.
  - `LookAt::Upgrade(slot, id)`, `Cores::State(team)`, `Cores::SavePosition(team, origin)`.
  - voltmod: `runtime.World.Rounds.End(VoltMod::RoundEndReason, delaySeconds)`; `VoltMod::Schema::CBeam{entity}` with `SetWidth`, `SetEndWidth`, `SetEndPos`; `ServerCommand` has a `(const CCommand&, int slot)` overload that is client-executable. A `.Console()` command typed in a player's console now runs as that player, permissions included. `ConsoleOnly()` commands, `volt` and the anticheat simulator commands ignore players.
- Deviations:
  - The command fix took three voltmod commits. The first made `ServerCommand::Handler` take the slot, which broke anticheat's simulator, so the second kept the one-argument handler for the server only. The third added `FCVAR_CLIENT_CAN_EXECUTE`, without which the engine refuses a client's call before any handler runs. The net API change is additive, so the `!` on `a1ec6ec` no longer applies.
  - Two framework additions the step did not list, both general:
    - Round end: `endround` is a cheat command, and CS2's `server.dll` has no `game_round_end` entity (checked by string search), so there was no convar or entity route. `Rounds::End` calls `CCSGameRules::TerminateRound`; the signature is CS2Fixes' (SwiftlyS2 has the same bytes). The game rules pointer comes from `cs_gamerules` `m_pGameRules`, added to the schema manifest. Team scores are not changed.
    - Beam: a spawned `env_beam` takes the server down 2 to 4 seconds later, with no minidump. That held for both a minimal variant (`targetpoint`) and the full one (`LightningStart`/`LightningEnd` to an `info_target`). The laser draws a `beam` entity instead: `CreateByName("beam")`, set `m_fWidth`, `m_fEndWidth` and `m_vecEndPos` through the new `CBeam` view, `SetRender` for the colour, then `DispatchSpawn` with the origin. The beam uses its default material, so there is no texture setting.
  - Turret: no tracer or muzzle particle, since F4 is still unproven and a particle cannot be seen headless. The level 2 and 3 `spin` animation is not played. The head is the last part, lifted `turret.headHeight` above the stand by `Teleport` and turned by `Teleport` angles each frame.
  - Class names are plural where a class runs every instance: `Turrets`, `LaserMines`. The pure round logic is `RoundRules`.
  - `sh_core_set` needs `admin.map`, and no plugin installs `Policy.HasPermission` for stronghold: admin-system's policy lives in its own runtime. It is refused in game. The command takes an optional player, so the server console or RCON can set it: `sh_core_set t <player>`.
  - The Core is the `core` item: the tesla model at `scale` 3, 20000 health, never sold. Defaults: `cores.damageRadius` 1500, `roundSeconds` 900, `restartDelaySeconds` 7, `carryOverShare` 0.5. The plugin's timer ends the round, so `mp_roundtime` 60 stays as a backstop.
  - Money is carried over at the first round start after a `round_end`, because the engine fires `round_start` twice at map start. The summary goes to chat: the result, the top builder and the top destroyer.
  - main-menu has six tabs, the most it allows, so the `stronghold` section entry is the first row of the Stats tab.
  - Precache and addon: `Addons.Require(addonId)` runs at load when the id is not 0. There is no separate center-text fallback for a client still downloading; the shop is a framework menu, which already falls back to center HTML.
- Findings:
  - The turret stand's top is 39.1 units above the placement point (trace onto the spawned stand), so `headHeight` is 40.
  - Turret and laser kills: `player_death` has `attacker` = owner and `weapon=prop_dynamic`. The engine names the inflictor's class, not the owner's gun as the F1 spike's pawn-inflictor kill did.
  - `Rounds.End` on de_dust2: `round_end winner=2 reason=9` (T) and `winner=3 reason=8` (CT), with the next round after the delay.
  - `voltmod gamedata check` passes 24/24 on Windows build 2000908, the new TerminateRound pattern included. The schema layout check passed with `CCSGameRulesProxy` and `CBeam`.
  - `find sh_shop` lists only `game server_can_execute` even with `FCVAR_CLIENT_CAN_EXECUTE` set. The client path was proven by the menu opening.
  - After `volt load stronghold` mid-map, the plugin's roster was empty: a `#slot` target resolved to nobody. It was fine after a server restart.
- Live smoke test (local server, de_dust2, bots, through a temporary `sh_probe` console command that was not committed):
  - `sh_shop` run in a bot's client context (`ExecuteClientCommand`) opened the shop: "Menu opened for slot 4 (title: Shop ...)". From RCON it replies "Only players can open the shop.". `sh_core_set t` in a bot's client context was denied by the permission check, not run as the server.
  - Turrets built next to enemy bots killed them: 11 kills credited to the owner in about a minute, turret frags 3, 3, 2 and 2, and the owner's balance rose with the kill rewards.
  - Laser mines placed on corridor walls killed crossing enemies, with the owner credited and the mine's frags counted. A mine upgraded to level 2 kept its beam and went on killing.
  - Upgrades charged 1200 then 2000 for a turret and 800 for a mine. The head model changed to `level2.vmdl`, then `level3.vmdl` (read back), health refilled to 900 and then 1200, and a fourth press was refused at the maximum level. The upgrader bonus was paid: 17000 + 800 kill + 100 bonus + 800 = 18700.
  - Cores:
    - Both spawned at round start.
    - 25000 damage from an enemy standing far away was blocked (20000 → 20000).
    - From close by it destroyed the T Core, and `round_end winner=3` followed. The new round began 7 s later with fresh Cores, and every balance was cut to max(start money, half).
    - With `roundSeconds` 45 and the CT Core at 15000/20000, the round ended at 45 s with T winning (`reason=9`).
    - A position saved from the console (`sh_core_set t #5`) was used by the next round's Core.
  - Addon: with `addonId` set, `Require` installed the workshop download and mount hooks with no warning. The local server is back to 0.
- Performance (30 bots, since the fill stopped at 30 on de_dust2; 60 turrets, 4 laser mines and 2 Cores; temporary timers, reverted):
  - The frame loop held 64 calls a second.
  - The turret loop took 46 to 66 µs per frame on average (90 to 100 µs in the busiest window).
  - A shot that does not kill takes 40 to 50 µs. A shot that kills takes 0.35 to 4.5 ms, which is the engine's death handling inside `Damage.Apply`; frame peaks of 3 to 10 ms line up with kills.
  - The laser tick took 4 to 5 µs for 4 mines at 20 Hz.
- Linux, to verify before any Linux deploy:
  - the `CCSGameRules::TerminateRound` pattern (CS2Fixes' bytes);
  - the inferred `CCSGameRulesProxy` and `CBeam` layouts in `schema/server.linux.json` (the Windows offsets + 736, the gap every entity class shows). A wrong entry stops the framework from loading.
- User must check in game:
  - the turret head sits on the stand and turns and pitches onto targets, and the red and blue material groups show and survive an upgrade's `SetModel`;
  - the kill feed for a turret or laser kill (owner name, and the icon for `prop_dynamic`), and whether the fire sound is too loud or too frequent;
  - the laser beam: visible, team-coloured, starting at the emitter, ending at the wall, gone while the owner is dead;
  - the look-at center text (line breaks), E upgrades from a real client, no upgrade while a menu is open, and the "Not enough money" reply;
  - the Core's size and where it lands at the spawn centroid on each map (players may spawn inside a scaled Core, so save positions with `sh_core_set`); the "Core under attack" center and chat alert; the round summary and win panel;
  - `sh_shop` typed in the console and `bind b sh_shop`;
  - the "Stronghold shop" row in `!menu`. The server's main-menu `settings.jsonc` is seeded once, so copy the new one first.
- Blockers: `sh_core_set` cannot be used in game until stronghold gets a permission policy. That needs a cross-plugin permission contract from admin-system, which M4's VIP tiers need too. Everything else in part B is done. The HUD, the tutorial and the anticheat/bhop checks remain open.

### 2026-09-21 — Milestone 3 signature features (step 4, part A)

- Landed in `plugins/stronghold` on `feat/stronghold` (not pushed). Nothing changed in voltmod, and `conan.lock` is not relocked; the root still builds against `conan editable add voltmod`.
  - `940e1ea` feat: turn an enemy structure by holding E behind it, and let crouching enemies slip past turrets
  - `87fd381` feat: add sensor towers that show enemies in range through walls to their team
  - `0d0e366` feat: add teleporter pairs placed as an entrance and an exit for one price
  - `ec686db` feat: add jump pads that launch allies along their facing and spare them the landing
  - `fe8d769` feat: let a turret's last upgrade specialise it as Gatling, Marksman or Cryo
  - `37ff80c` refactor: give the item settings their own header
  - `29704a0` feat: drop scrap from destroyed structures, put bounties on streaks and big bases, and boost the trailing team
  - `534611e` feat: drop timed supply crates that give a free level 2 structure to whoever claims them
  - `dbfcfdb` refactor: share message, item name and sound helpers, and look up placement prompts from a table
- New files (`src/`): `Sabotage`, `SensorTowers`, `Teleporters`, `JumpPads`, `TurretBranches`, `Scrap`, `Bounties`, `SupplyDrops`, `ItemSettings` (split out of `Config`), `Text`, and the SDK-free `Hold`, `PadRules` and `Rewards`. `DropPoint` joined `RoundRules`, and `InCone` joined `TurretAim`. New tests: `HoldTests`, `PadRulesTests` and `RewardsTests`, plus cases in `TurretAimTests` and `RoundRulesTests`. The repo runs 257 CTest cases, all passing; `poe build`, `poe test` and `poe lint` pass. The largest files are `Placement.cpp` (290) and `Structures.cpp` (223).
- API for the next step:
  - `Structures::Build(item, slot, origin, angles, level = 1)` starts a structure at any level, with that level's head. `Structures::LevelUp(structure, item, next, upgraderSlot)` applies a level or a branch. `StatsOf(item, structure)` gives the branch's stats when one was taken, else the level's. Also `IsBuilt(structure, item, now)`, `PlaySound(runtime, ref, soundEvent)` and `Structures::SetPowered(Structure&, bool)`.
  - `Structure` gained `Branch`, `PairId`, `Exit` and `PlacedAt`. `CountOwned` skips teleporter exits.
  - `Placement::Begin(slot, item, PlacementOrder{Free, Level, EntranceId, Refund})`. A free order charges nothing. Cancelling a teleporter exit removes its entrance and refunds it.
  - `ForSale(settings, item)` hides Cores and items whose feature is off; the shop and supply drops use it. `FindBranch(item, id)`.
  - `Text.hpp`: `ItemName(runtime, slot, itemId)`, `Tell(runtime, slot, key, tokens, kind)` and `TellAll(runtime, key, tokens, kind)`.
  - `PadRules`: `OnPad`, `LaunchVelocity`, `Cooldowns` (`TryUse`, `Ready`, `ForgetReady`) and `SteadyMs`. `Hold`: `HoldShare` and `ProgressBar`. `Rewards`: `ScrapValue`, `BountyFor`, `KillReward` and `BountySettings`.
  - `Cores::DropPoint()`. `configs/cores/<map>.json` takes an optional `drop` point, written by hand.
  - `StructureHealth::Destroy(structure, destroyerSlot)` is public and drops scrap. `Bounties::OnDeath` now pays every kill reward; that code moved out of `App`.
  - Settings: new `sabotage`, `scrap`, `bounties` and `supplyDrops` sections; per item `branches`, `pad` and `sounds.use`; per level `rechargeMs`; `turret.crouchConeDegrees`. New item kinds: `sensor_tower`, `teleporter` and `jump_pad`.
- Deviations:
  - Progress bars and prompts are center text (`[####----]`), not a Panorama HUD, which does not exist yet.
  - Sabotage: the owner is warned in chat and center text when the hold starts. The turned structure changes owner as well as team, so its kills and its power follow the saboteur, and its upgraders are dropped. When its time is up it goes through `StructureHealth::Destroy`, so it drops scrap and pays nobody. Cores cannot be sabotaged. A laser mine faces out of its wall, so nobody can stand behind it; it can still be shot. The turned structure keeps its material group.
  - Turrets ignore a crouching enemy outside `crouchConeDegrees` of the way the turret was placed facing, not of where the head points, and only while `features.sabotage` is on.
  - Sensor tower: the pole model. Only humans get a glow, since bots cannot see it and every glow costs entities.
  - The teleporter and the jump pad both use the landmine model (scale 2 and 1.5). There is no teleporter model and no team-coloured particle. Losing either teleporter half removes the other on the next tick, with no scrap for it.
  - Jump pad stats are per item (`pad`), not per level. The pad writes the velocity with `SetVelocity` and clears `FL_ONGROUND`, as bhop does. `Teleport` with no origin moved nothing, and voltmod's `Pawns::Slap` notes that it has crashed CS2.
  - Turret branches: the choice is a framework menu that opens on the E press for the last upgrade, since the look-at panel is center text and has no buttons. All three branches use the `level3.vmdl` head.
  - Scrap uses the drone model and has no collision. The pickup sound plays from the taker, because the piece is removed at once.
  - Bounties: the clan tag is `$<amount>`. While a bounty shows, the controller's `m_szClan` points into a string the plugin owns. The player's own tag is put back when the bounty ends, on disconnect and on unload. A client that changes its clan tag meanwhile overwrites ours until the bounty changes. The trailing boost needs `features.cores`.
  - Supply drops: the crate is `money_dispenser.vmdl` with no collision, moved down by `Teleport` each frame. The beacon is the `Stronghold.Air.Siren` sound only, with no particle. A claim means standing within `claimDistance` and holding E; no aiming is needed. The drop point can only be written by hand; there is no command for it.
- Findings:
  - Bots crouched with `bot_crouch 1` have `FL_DUCKING` (2) set in `m_fFlags`.
  - A bot teleported into the air hovers until it moves on its own, and one teleported into a prop's collision is stuck. Pad tests therefore drop bots a little above the pad.
  - A settings file with a UTF-8 byte order mark fails to load (`1:1: expected_brace`). PowerShell's `Set-Content -Encoding utf8` writes one.
- Live smoke test (local server, de_dust2, bots, through a temporary `sh_probe` console command that was not committed):
  - Sabotage: with a bot 60 units straight behind a turret, `CanSabotage` was true. It was false in front, at 300 units and for a teammate. A forced turn moved the turret to the saboteur's team and owner and kept its frags. It shot its old team (100 → 12 hp) and destroyed itself when `switchedMs` ran out (8 s for the test).
  - Crouch rule: a crouched enemy 250 units behind a turret took no damage for 4 s. 250 units in front, it was shot (100 → 36).
  - Sensor tower (bots allowed as viewers in the probe build only): after the build time, `prop_dynamic` went from 5 to 11 (3 viewers × 1 enemy × 2 clones). Moving the enemy out of range took it back to 5, and back in range to 11. Killing the owner powered the tower down and removed the clones.
  - Teleporter (halves built and linked by the probe): a bot on the entrance arrived on the exit, 16 units up. It stayed put through the 8 s recharge and while another bot stood on the exit, then went once the exit was clear. Removing the exit removed the entrance.
  - Jump pad: a bot dropped onto the pad left at (649, 27, 287) u/s along the pad's yaw and rose about 200 units. Fall damage inside the window was blocked (40 → 40 hp); a bullet was not (40 → 25).
  - Turret branches: the level 2 upgrade charged 1200. The next upgrade opened "Choose the level 3 turret" with three rows and charged nothing. With Cryo set by the probe, a hit enemy's max speed went from 260 to 130, and back to 260 about 1.5 s after the last hit.
  - Scrap: an enemy who destroyed a wall got +500, and a scrap piece replaced the wall. Walking onto it paid +450 (1500 × 0.3) and removed it.
  - Bounties (test settings `streakFrom` 1 and `baseFrom` 2): a kill gave +800 and the tag `$400`. Killing that bot paid +1200 (800 + 400), and its own tag was back after respawn. Two walls gave their owner `$400`. With the T Core at 50%, a T kill paid 1200 (800 × 1.5).
  - Supply drops (`intervalSeconds` 20, `lifetimeSeconds` 8): nothing came for the first 20 s. Then a crate appeared 800 units above the midpoint of the Cores, fell for 3 s onto the floor, and went 8 s after landing. A claim forced by the probe put the claiming bot into placement mode and removed the crate.
  - After the refactor, the committed build loaded cleanly and ran with 10 bots. The only load warning is the known `sh_core_set` permission.
- User must check in game:
  - Sabotage: holding E behind an enemy structure, the progress bar, the owner's warning, and that the hold stops when you are hit, let go, step away or look away. Also the `[Hold E] Sabotage` line in the panel.
  - Sensor tower: the glow through walls, shown to your team only, and whether the pole reads as a tower.
  - Teleporter: the two-step placement (entrance, then exit; R on the exit refunds), the "Now place the exit" message, and the trip itself. Check the landmine model at scale 2.
  - Jump pad: how the launch feels (`launchSpeed` 650, `launchUp` 550), no fall damage on landing, and the 1.5 s cooldown.
  - Turret branches: the menu from E on a level 2 turret, its price rows, the panel line `Turret · Cryo`, and the Cryo slow on a real player. Check whether a weapon switch resets the slowed max speed.
  - Scrap: the drone model on the floor, and the pickup message and sound.
  - Bounties: the `$600` clan tag on the scoreboard, and the chat messages when a bounty is placed and claimed.
  - Supply drops: the fall, the siren, the center progress while holding E, and the free placement (the `· free` prompt). On de_dust2 the midpoint of the Cores is in mid (-238, 739, 0). Other maps may need a `drop` point in `configs/cores/<map>.json`.
- Blockers: none. Still open in M3 (part B): landmine, dispensers, tesla coil, perks and the balance pass.

### 2026-09-21 — Milestone 3 structures and perks (step 4, part B)

- Landed (not pushed, not tagged, `conan.lock` not relocked; the root still builds against `conan editable add voltmod`):
  - voltmod `feat/stronghold`: `e66959b` feat: generate the entity owner handle, such as a thrown grenade's thrower. `m_hOwnerEntity` becomes `OwnerHandle()`/`SetOwnerHandle()` on every wrapper, with a line in `docs/sdk/entities.md` on resolving a handle field. The schema layout stamp changed, so every plugin needs a rebuild against this checkout (`poe build --install-all`).
  - stronghold `feat/stronghold`:
    - `61102db` refactor: share the check for a working structure of a kind, and name the per-team pair plainly
    - `e84854a` feat: add landmines that blast enemies who step on them for their owner
    - `721bdb7` feat: add health dispensers that heal allies in range, dressed by team and level
    - `b742aa7` feat: add money dispensers that pay allies in range
    - `ee6e643` feat: add tesla coils that repair allied structures and destroy enemy grenades in range
    - `22ec542` feat: sell speed, gravity, health, income and regen perks from a Perks shop page
    - `a5e304a` fix: raise max speed with every speed level and put low gravity back after a respawn
- New files (`src/`): `Landmines`, `Dispensers` (health and money), `TeslaCoils`, `Perks`, and the SDK-free `Blast` and `PerkRules`. New tests: `BlastTests` and `PerkRulesTests`. The repo runs 266 CTest cases, all passing; `poe build`, `poe test` and `poe lint` pass. The largest files are `Placement.cpp` (290) and `Structures.cpp` (286); nothing is over 300.
- API for the next steps:
  - `WorkingItem(settings, structure, kind, now)` returns the item of a powered, built structure of that kind, else null. Sensor towers, jump pads, landmines, dispensers and tesla coils use it.
  - Item kinds `landmine`, `health_dispenser`, `money_dispenser` and `tesla_coil`. Per level: `amount` and `intervalMs` (a dispenser's gift, a coil's repair). Per item: `particles {t, ct}` (a looping particle at the origin, removed with the structure) and `mine.edgeDamageShare`; `pad` doubles as a landmine's trigger. `SkinSettings` is now `TeamNames`, and a skin may hold `{level}` (`t_{level}`). A level that changes the group respawns the last part, so `Parts.back()` gets a new ref.
  - `Perks`: `Buy(slot, Perk)` returns a `PerkPurchase`; also `Level`, `SettingsOf`, `IncomeFor(slot, reward)`, `OnDeath`, `Forget` and `ForgetAll`. `PerkRules`: `SpeedScale`, `GravityScale`, `WithIncome`, `BoostedMaxHealth` and `Regenerated`. Settings: `perks {speed, gravity, health, income, regen: {price, maxLevel, step}, regenCap, baseHealth, baseMaxSpeed}`. `Bounties` takes `Perks&` and pays kill rewards through `IncomeFor`; bounties are not raised.
- Decisions and deviations:
  - Perk reset rule: the health perk ends at death. Speed, gravity, income and regen last until the player leaves or the map changes; rounds do not reset them.
  - Perks have their own shop page ("Perks") between Structures and VIP, not a "Shop" tab that mixes perks and objects. Perk names carry no numbers, so changing a step in config does not make the menu wrong; the README table gives the defaults.
  - Speed perk: `m_flMaxspeed` alone does not make anyone faster, because the weapon's speed caps movement (bots with max speed 330 still peaked at 215 with an AK). What works is holding the velocity modifier (`SetSpeedModifier`) at 1 + step × level every frame, plus raising max speed by the same factor (otherwise 260 caps it). The modifier is raised only while it is at least 1, so the engine's slow after a hit still applies. Max speed is raised only from the engine's 260 or from the value the perk last set, so Cryo's slow is left alone.
  - Gravity is also kept every frame. The engine resets it on respawn after `player_spawn` fires; the loadout's delayed give has the same cause. Max health and max speed also reset on respawn.
  - Landmine: the blast is a radius `Damage.Apply` through `StructureAttack::Strike` (`DamageBlast`), measured to the waist, with linear falloff to `edgeDamageShare`. It hits enemies only, with no line-of-sight check and no damage to structures. The mine is removed after the blast without scrap. A mine shot to 0 breaks through `StructureHealth` and drops scrap. The arm delay is `buildMs` (2 s). The blast sound plays from the player who stepped on the mine, because the mine is removed at once. There is no explosion particle; the pack has none.
  - Particles: the dispenser transfer particle and the tesla arc and zap particles are skipped. They draw a path between two control points (`C_INIT_CreateSequentialPathV2`), so they need a second point set on the `info_particle_system`. The mine beacon is a plain continuous emitter and works as an item particle. The dispenser sound plays once per gift interval when anyone took something.
  - Tesla coil: Cores are not repaired. A grenade's team is its owner entity's (the thrower's pawn), else the projectile's own team. The grenade is removed and the coil plays `sounds.fire` (`Stronghold.Air.Intercept`); there is no zap particle.
  - The placement boxes for the dispenser, money dispenser and tesla coil are guesses (`[16,16,56]`, `[16,16,72]`).
  - Balance: every number is in `settings.jsonc`. Prices and limits follow plan section 2. Levels: landmine blast 150/200 damage over 220/280 units; health dispenser 5/8/12 hp a second over 250/300/350 units; money dispenser $30/45/60 every 5 s; tesla repair 20/35/50 a second over 400/500/600 units. The "balance pass on a live test server" box stays open, because it needs human games.
- Findings:
  - A thrown HE grenade's `m_hOwnerEntity` is the thrower's `player` pawn, and the projectile's `m_iTeamNum` is also the thrower's team (bot throw on de_dust2).
  - `info_particle_system` with `effect_name`, `start_active` 1 and an origin spawns, lives, and did not disturb the server (one for 10 s, then 8 beacons for minutes). Seeing it needs a client.
  - Respawn resets `m_flGravityScale` to 1, `m_iMaxHealth` to 100 and max speed to 260. A weapon switch (strip, then knife and AWP, then Negev) keeps a raised max speed.
  - With speed modifier 1.3 alone, AK bots peaked at 254, capped by max speed 260. With max speed 400 as well, one reached 276 (215 × 1.3 ≈ 280).
- Live smoke test (local server, de_dust2, 8 bots, `bot_stop 1` for the aura checks, through a temporary `sh_probe` console command that was not committed):
  - Landmines: all 8 spawned with their beacon particle. An enemy put on an armed mine took the blast (100 → 23, with armor) and the mine was gone. At 10 hp the enemy died, and the owner was paid +800. A mine shot for 50 broke. Bots set off another mine on their own.
  - Health dispenser: an ally at 50 hp healed 5 a second up to 100 and stopped there. Upgrading the dispenser respawned its part (`prop_dynamic#337` → `#343`), and a shot on the new part took 30 health.
  - Money dispenser: an ally in range got +$30 every 5 s. Each of two upgrades respawned the part.
  - Tesla coil: a dispenser 200 below full was repaired 20 a second. An HE, a molotov and a flash spawned beside the coil with an enemy owner were gone within 300 ms. An HE owned by an ally stayed.
  - Perks on a bot:
    - Speed ×3 set the modifier to 1.30 and max speed to 338 (after the fix; before it, max speed stayed at level 1's 286). A fourth purchase was refused as maxed.
    - Gravity ×2 gave 0.80, health ×2 gave 150 max health, and regen ×1 took 50 → 53 over 3 s. Income ×1 paid 880 for a kill.
    - After death and respawn: 100/100 health (the health perk was gone), and gravity 0.80, max speed 338 and modifier 1.30 were back. The bot's sampled speed reached 254 with an AK.
- User must check in game:
  - Speed perk on a human: that 1.1–1.3× feels right, survives weapon switches and scoping, and that anticheat and bhop leave it alone.
  - Gravity perk: the jump height at 0.9–0.7 gravity.
  - Health perk: that the HUD shows 125–300 health and does not clamp it.
  - Landmine: the model on the floor, the beacon particle and its team colour, the blast sound, and the kill feed for a mine kill.
  - Dispensers: the `ct_1..3`/`t_1..3` and `level_1..3` material groups (the `skin` keyvalue takes the group's name, which has never been seen in a client), and whether the use sound every second is too much.
  - Tesla coil: grenades vanishing mid-air near an enemy coil, the intercept sound, and a coil repairing a wall under fire.
  - The Perks page rows and replies.
- Blockers: none. Still open in M3: the live balance pass. The F4 particle box stays open for the in-game check above and for two-control-point particles (dispenser transfer, tesla arcs).

### 2026-09-21 — Integration: anticheat and bhop (step 6, part)

- Landed (not pushed): anticheat `feat/stronghold` (new branch from `main` at `9c2c134`; the submodule was on `main`, not `anticheat-legit`) `8191fc3` fix: keep grenade and plugin damage from being paired with the attacker's shots. Root: this entry and the submodule pointer. Nothing changed in voltmod, contracts, stronghold or bhop.
- What could false-flag, and what was done:
  - **Turret, laser and landmine hits.** `Damage.Apply` credits the owner, so `player_hurt` names the owner as attacker. The anticheat paired every `player_hurt` with the attacker's shot fired in the same or previous tick (`ShotHistory::OnPlayerHurt`). An owner shooting while their turret hit someone got a "hit" on a victim their gun never touched, which feeds aimbot (snap onto the victim), triggerbot, silent aim, and wallhack ("shot through cover hit an enemy nobody could see"). Fix: a new `Engine/IndirectDamage` subscribes to `Hooks.Damage.Before`. For a player victim whose inflictor is not the attacker, it records (attacker, victim, tick) in the SDK-free `Detect/IndirectHits`. `DetectionFeed::OnPlayerHurt` takes that record back and skips the event, because `player_hurt` fires inside the same `TakeDamageOld` call and server-side listeners run synchronously. This is general: grenades and molotovs are skipped the same way. It needs no contract. The spike showed gun damage names the shooter's pawn as both attacker and inflictor. `player_death` needed nothing, because it already matches on the weapon name, and a structure kill reports `prop_dynamic`.
  - **Teleporters.** `Teleport` goes through the pawn vtable, which voltmod's `Hooks.Teleport` already watches. So the existing 5 s teleport grace (samples marked `Teleported`, skipped by aimbot, wallhack, aim assist and antiaim) covers stronghold teleports with no change.
  - **Jump pads, speed, gravity and Cryo.** No change. The anticheat has no movement, speed or bhop detector. A launch or a speed change keeps origins continuous, and every aim rule measures angles against positions tick by tick. The one speed read (wallhack's victim speed) only makes it more lenient. Bhop servers already write velocity every hop with the anticheat loaded. A velocity-jump grace was left out, because it would open a detection hole for no false positive we can name.
  - **Bhop.** It tolerates the mode, so no change and no README note. It never touches max speed or gravity. Its hop boost only scales horizontal speed up (`min(speed × factor, max(maxSpeed, speed))`, never below the current speed). The auto-hop skips a pawn with upward velocity, so a pad launch is left alone. `sv_maxvelocity` is unchanged by default (-1). Whether to run bhop on Stronghold servers is a gameplay choice.
- Tests: `IndirectHitsTests` (4 cases). `anticheat-tests`: 182 cases pass. `poe lint` passes. `poe build` and `poe test` over the whole repo currently fail only on `stronghold-tests`: `VipRulesTests.cpp` from the concurrent VIP step has no `src/VipRules.cpp` yet (unresolved `TierPermission`, `HighestTier`, ...). The anticheat, anticheat-tests and bhop targets build clean against the voltmod checkout (conan editable).
- Live (local server, de_dust2): the new `anticheat.dll` loaded next to stronghold. Both logged "Damage hook installed", with no load failure, in observe mode. An end-to-end check was not done. A temporary console probe to fire `Damage.Apply` with a foreign inflictor was refused by the session's permission classifier, as a player-damaging command surface. Without a probe, bots place no structures. The other agent's server was also running from the same install at the time. For that run the server's anticheat config had `debug.includeBots` on; it has been restored to off. The installed `anticheat.dll` is the committed build.
- User must check (a human, since bots are not checked unless `debug.includeBots` is on):
  - In observe mode, own turrets and landmines while you shoot at other targets. `anticheat_status` should show no aimbot, triggerbot, silent-aim or wallhack evidence from structure kills.
  - Teleporters, jump pads and the speed and gravity perks should raise nothing either.
- Blockers: none for anticheat or bhop. The rest of step 6 (ru translations, the balance numbers) is the other agent's.

### 2026-09-21 — Milestone 4, VIP tiers (step 5)

- Landed (not pushed, not tagged, `conan.lock` not relocked; nothing changed in voltmod):
  - root `feat/stronghold`:
    - `56407e4` feat: publish admin-system's permission check for other plugins' policies
    - `6edac47` chore: seed the stronghold VIP tier groups and a test VIP
  - stronghold `feat/stronghold`:
    - `3fe363f` fix: take player permissions from admin-system, so admins can run sh_core_set in game
    - `13cb6f8` feat: add VIP tiers from stronghold.vip permissions with tags, colours, faster builds, salaries and a VIP page
  - root: this entry and the submodule pointer.
- Permission sharing:
  - voltmod has no host-level policy. Each plugin's `Runtime::Policy` is its own, and admin-system's `HasPermission` lived only in admin-system's runtime.
  - New contract `plugins/contracts/include/Contracts/IPermissions.hpp` (`cs2plugins.IPermissions/1`), with one method, `HasPermission(steamId, permission)`. admin-system publishes it from `Core/PermissionService.hpp`, next to `IAdminActions`, and unpublishes it in `~App`. It is backed by `Admin::Access`: group and wildcard permissions, minus abuse freezes.
  - stronghold's `App::InstallPolicy` sets `Runtime.Policy.HasPermission` to ask `Exchange.Get<IPermissions>()` on every call. Load order and an admin-system reload therefore do not matter, and with admin-system unloaded every permission is denied. Any other plugin can copy these few lines.
  - The "N command(s) gate on a permission with no HasPermission policy" load warning is gone.
- VIP design:
  - `VipRules` is SDK-free and tested by `VipRulesTests` (9 cases). It holds `VipTierSettings`, `VipSettings`, `TierPermission`, `HighestTier`, `ScaledBuildMs`, `DiscountedPrice`, `HasFreebies`, `CooldownLeft`, `MinutesAndSeconds` and `PackColor`. The laser beam's colour packing moved into `PackColor`.
  - `Vip` (`Vip.cpp`, `VipPage.cpp`):
    - `TierOf(slot)` asks `Policy::Authorize` for each tier, highest first, on every use, with no cache.
    - It also offers `PriceFor`, `LimitFor`, `ClanTag`, `Dress(structure, slot)` and `BuildPage(slot)`, and runs a salary timer and freebie claims.
  - `Structure` gained `BuildMs` (the item's build time, shortened by the tier), `Color` (the powered render colour) and `BeamColor`. `IsBuilt(structure, now)` no longer takes the item, and turrets and laser mines use the structure's build time. `Structures::Tint` repaints a structure.
  - Settings: `vip {salaryIntervalSeconds, freebieCooldownSeconds, tiers[]}` and `features.vip`.
    - Each tier has `clanTag`, `structureColor`, `beamColor`, `buildTimeShare`, `rememberLoadout` and `salary`.
    - The power options `extraLimit`, `discount`, `freeHealth`, `freeArmor` and `freeMoney` are all 0 by default.
    - The fair defaults are in the README table: tags `[VIP]`, `[VIP+]`, `[VIP++]` and `[ELITE]`, build time from 90% down to 70%, and $200 to $600 every 5 minutes.
- Decisions and deviations:
  - No database. admin-system groups hand out the tiers, and neither the groups nor the `admins` table have an expiry, so a VIP ends only when someone removes the membership by hand.
  - The "kill-feed tag" is the scoreboard clan tag. As far as is known, CS2's kill feed does not show clan tags; check in game. A bounty's `$amount` tag shows over it. `Bounties` owns both tags, so the two never fight over `m_szClan`.
  - "Structure skins" are a render tint (`structureColor`). Only turrets and dispensers have material groups, and those already carry team and level. The beam colour replaces the team colour on the owner's laser beams.
  - "Loadout presets remembered" keeps the chosen weapon set by SteamID in memory through reconnects and map changes, until the plugin reloads. There are no equipment presets.
  - Freebies are one claim row for all three gifts, with one cooldown. The cooldown is kept by SteamID, so reconnecting does not reset it. Health is raised up to max health and armor up to 100, and only a living player can claim.
  - Tiers do not add up; each lists everything it gives. Root (`*`) and `stronghold.*` hold every tier, so root admins get Extreme.
  - A member of a `vip_<tier>` group is an entry in admin-system's `admins` table, so admin-system treats them as an admin. They see the admin menu entry, which is empty for them, and get the group's `[VIP]` chat prefix. The seed groups have immunity 0 and no admin permissions.
  - Seed data in `plugins/admin-system/database/seed-admin.sql`: the groups `vip_basic` to `vip_extreme`, and a placeholder test VIP (SteamID `76561198000000001`, medium). Swap in a real second account to test with a client.
  - Deferred: persisting lifetime stats for a leaderboard (the last M4 box stays open), and the movement extras.
- Live smoke test (local server, de_dust2, 4 bots, driven through a temporary `sh_probe` console command that was not committed):
  - Setup, all undone afterwards:
    - Bots have SteamID 0, so a temporary `admins` row for SteamID 0 made every bot a medium VIP.
    - The test tier also had the power options on: limit +1, 10% off, 50 health, 100 armor and $1000. The salary ran every 20 s and the freebie cooldown was 60 s.
    - The row, the test config and the saved Core file were removed at the end.
  - The Permissions load step no longer warns. `Authorize(bot, "stronghold.vip.medium")` was true; `ultra` and `admin.map` were false. `TierOf` was medium: turret price 1800 (2000 less 10%), limit 3, and the clan tag `[VIP+]` on the controller.
  - A turret built for a VIP took 1600 ms instead of 2000, and a laser mine 1200 instead of 1500. Both got the tint `FFFFDCC8` and the beam colour `FF00C8FF` ([255, 200, 0]).
  - Salary: a stopped bot with no structures went 4200 → 4600 in one 20 s tick.
  - VIP page rows: "Your tier: Medium", "Claim your freebies", "What each tier gives", then Basic to Extreme, with "Medium (current)". Clicking the claim row paid +$1000 and set armor to 100. The page then read "Freebies ready in 1:00", and a second click gave nothing.
  - Remembered weapon set: choosing the AWP for one bot made every other bot report `set=awp`, since they share SteamID 0.
  - `sh_core_set`:
    - Run from a bot's client as a medium VIP, `sh_core_set t` was refused, and no cores file was written.
    - Then the SteamID 0 row moved to the `admin` group, and admin-system was reloaded (`volt reload admin-system`). stronghold's policy answered `admin.map` true through the republished contract, and the same command saved `configs/cores/de_dust2.json`.
    - The bot's tier went to none, and its clan tag went back to its own.
  - With admin-system unloaded, `admin.map` was false. After `volt load admin-system` it was true again.
- Findings:
  - Bots stay in `challenging` and never join until `bot_join_after_player 0` is set. With no human connected, this looked like a server fault.
  - `meta unload 1` unloaded every plugin, then hung the server, which had to be killed.
  - `admin_reload` has no console form. From RCON, `volt reload admin-system` is the way to pick up a database edit.
  - Another session was using the local server during this step, so this test ran on its own server start. The local server's stronghold `settings.jsonc` was replaced with the repo's copy, which adds the `vip` section; the seeded copy had no other local edits.
- User must check in game:
  - The VIP page in the shop, with a real VIP account: swap the seed's placeholder SteamID for a second account, then run `!admin_reload`. Also check the claim row once a power option is on.
  - The clan tag on the scoreboard, and whether any tag reaches the kill feed.
  - The structure tint on each model, and the gold, violet and teal beam colours.
  - The VIP salary chat line every 5 minutes.
  - `sh_core_set` from a real admin client, and its refusal for a normal player.
- Blockers: none. This resolves the M2 part B blocker, where stronghold had no permission policy.

### 2026-09-21 — Milestone 5, air and armor (step 7)

- Landed (not pushed, not tagged, `conan.lock` not relocked). Nothing changed in voltmod: the camera fields from step 1 were enough.
  - stronghold `feat/stronghold`:
    - `e6db1b8` feat: add rocket batteries that fire one warned salvo of arcing rockets at an aimed point
    - `84b3df0` fix: let rockets fly over ceilings, trace them like sight lines, and clear their remains without the engine's delayed kill
    - `b3486f6` feat: add air defenses that fire interceptors at enemy rockets and flying drones
    - `55b5198` feat: add nets whose square catches enemy drones that fly under them
    - `7880c76` refactor: deal blasts and hits on structures through StructureAttack, for every structure that fires
    - `35567c4` fix: burst a rocket that falls far below its target through a gap in the map
    - `cfa1027` feat: add scout and gun drones that their owner flies through a camera, with a kamikaze blast and a gun
    - `1ce2343` feat: add a drivable tank whose turret follows the driver's view and fires bursting shells
    - `75793a6` refactor: share the capped frame clock between turrets, shots and vehicles
    - `c2df9bf` docs: describe rocket batteries, air defenses, nets, drones and tanks
  - root: this entry, the ticked boxes and the submodule pointer.
- Files (`src/`): `Ballistics` (SDK-free arc math), `Projectiles` (every rocket, shell and interceptor in flight), `RocketBatteries`, `AirDefense`, `Nets` (SDK-free catch zone), `FrameClock.hpp`, and `Vehicles/` (`Pilots`, `Drones`, `Tanks`, SDK-free `DriveRules`). `StructureAttack` gained `StrikePart` and `Blast`. Tests: `BallisticsTests`, `NetsTests`, `DriveRulesTests`; the repo runs 295 CTest cases, all passing. `poe build`, `poe test` and `poe lint` pass. Largest files: `Placement.cpp` 301, `ItemSettings.hpp` 299, `Structures.cpp` 294, `Projectiles.cpp` 279.
- F5 gate: **passed as "plausible, user must verify"**.
  - Recipe (1), CS2Fixes' view entity: `pawn.CameraServices().SetViewEntity(camera)` on an invisible `prop_dynamic`, `ZoomOwner` set to the invalid handle, optional `FieldOfView`. On a bot the engine accepted it with no error, the fields read back unchanged seconds later, and the pawn's `EyeAngles` followed the camera's angles (0,0, then 30,90 after a `Teleport` of the camera). `SetViewEntity(invalid)` and `MoveType::Walk` gave the view back.
  - Approach (2), observer mode on a live pawn: `SetObserverMode` refused (a live pawn's observer services are null). Approach (3), hiding the pawn, was not needed.
  - Input: `MoveType::None` plus `Movement.Before` is enough. A seated bot with `bot_stop 0` steered its drone and tank with its own usercmds; no writeback was needed. A forced input from a test probe drove the vehicles the rest of the time.
  - Whether a real client sees through the camera, and whether its usercmd view angles still turn with the mouse while the view entity is set, can only be checked in game. If the client pins its angles to the camera's, steering needs `MouseDx/MouseDy` instead (both are already decoded in `PlayerInput`).
- Decisions and deviations:
  - Rocket battery: E on your own built battery starts aiming (center prompt with range), E fires, R cancels. The sky check is a line straight up of `placement.skyClearance` (300; de_dust2's sky ceiling over mid is only about 448 above the floor). Rockets fly the highest arc under `projectile.maxRise` and pass through everything until they fall below `collideHeight` (200) above their target, so the map's sky ceiling and roofs do not stop them. They trace the sight layer, which still hits structures, so a player clip over CT spawn does not catch them. A rocket that falls 400 below its target without a hit bursts there. The battery stays until its last rocket lands, so every blast is credited, then removes itself with no scrap. Blasts hurt enemy players (through `Strike`) and enemy structures (through the damage hook, so a far Core takes nothing).
  - The engine's delayed `Kill` (`EntityOps::RemoveDelayed`) crashed the server with no minidump: at once from an RCON command, for a fresh `prop_dynamic` and a fresh `info_particle_system`, and a moment later from the frame loop for impact particles. Stronghold removes lingering props and particles itself (`Projectiles::Linger`). The framework method itself was left alone; it needs a look before anyone else uses it.
  - Air defense: one interceptor per launcher per `fireIntervalMs`, no two launchers on one rocket in the same tick. The upper parts turn to the target's yaw when firing. Interceptors home in straight (no gravity) and destroy a rocket on contact, or deal the level's damage to a drone through its owner.
  - Net: the net model's four physics hulls block nothing (no trace hit it at any height), so a net is a catch zone, a square of `net.halfWidth` (164, from the model's hull data) up to `net.height` (150), turned with the net. The pole is its first, shootable part. An enemy drone that enters the zone is destroyed with the net's owner credited.
  - Vehicles are structures: placed through the normal placement, with health, look-at panel, scrap and cleanup. The owner gets in with E (`[E] Get in` in the panel). The pilot's pawn is frozen where it stands and can be shot (vulnerable was chosen). On entry the pilot is switched to the knife (`use weapon_knife`), so attack does not spend their ammunition. The camera prop needs `DisableCollision`: with only `solid` 0 it blocked traces, the drone's own shots included.
  - Drones: velocity steering (forward and strafe along the view yaw, jump and crouch to climb and sink) with a line trace along the way it flies (a hull trace from the drone reported start-solid, since the camera prop sat inside it). A flight uses the drone up when it ends: the pilot gets out or dies, the battery runs out (`lifetimeMs` 60 s scout, 45 s gun), a net catches it, or it is shot down. Scout: attack is the kamikaze blast (250 over 300). Gun drone: 12 damage every 150 ms out to 2000. No spotting markers (optional, skipped).
  - Tank: five parts (hull, two tracks, turret, gun) moved by `Teleport` every frame from attachment offsets read out of the models (`hullParts`, `gun`, `muzzle` in config). The hull turns with left and right, drives with forward and back (reverse at half speed), follows the ground with a downward trace and stops when a bumper line hits something. The turret turns toward the view yaw at `turretTurnRate`; the gun follows the turret but does not pitch (shells use the view pitch). The shell flies with light gravity and bursts (150 over 220). Health 3000. No crush damage. A parked tank keeps its place and turret for its owner.
  - Measured on spawned props: the hull top is 69 above its base and the turret top 99.4 with the offsets above; the gun model extends forward along +x, so `gunYawOffset` is 0 (the model's `muzzle` attachment reads `[0, -178.8, 0]` but its hull runs 0..178.8 along x).
- Live smoke test (local server, de_dust2, 6 to 10 bots, a temporary `sh_probe` console command that was not committed):
  - A salvo from mid onto two stationary CT bots killed both with the first rocket; `killed ... with "prop_dynamic"` credited the battery's owner. The battery was gone after the last rocket.
  - With a CT air defense beside the target, several rockets of a salvo never landed (intercepted); two CT bots still took 70 to 76.
  - A T air defense shot a flying CT drone down, and the pilot got their view and movement back. A test structure marked airborne lost 200 per interceptor.
  - A CT gun drone flew at about 390 u/s, climbed, and hit a T bot six times (100 → 28). A T gun drone flying into a CT net's square was destroyed. A scout drone's blast killed two CT bots next to it.
  - A tank drove 260 units forward, stopped at the net's pole in front of its bumper, and a shell took a CT bot 100 → 51. Getting out and back in worked.
  - After the final refactor the committed build loaded cleanly and a salvo killed two bots again.
- Performance (per frame, 64 ticks, averaged over 640 frames, temporary timers not committed):
  - 3 batteries firing 24 rockets in 10 s, 2 air defenses, a tank and drones driven by bots: `Projectiles` 13 to 21 µs average (peaks 150 to 720 µs, the engine's death handling inside a kill), `RocketBatteries` 3 to 5 µs, `Tanks` 8 to 11 µs, `Drones` 12 µs with three drones flying, `Pilots` 0.7 µs.
- Linux: nothing new to verify; no signature or schema field was added in this step.
- User must check in game (also in the overall checklist below): the view through the camera for drones and the tank, mouse steering while seated, WASD, jump and crouch, E to get in and out, the knife switch; the rocket model's facing along its arc, the exhaust trail and the stock explosion; the warning banner and siren; the air defense turning and its interceptor; the net and pole models together at the right height; the tank's parts lined up, the gun on the turret, and the chase camera distance.
- Blockers: none. The F5 view itself needs a human in a client.

### 2026-09-21 — Overall status

- Done on `feat/stronghold` in voltmod, `plugins/stronghold`, `plugins/anticheat` and the root: M0 assets, M1 framework APIs (damage hook and apply, trace hit entity, normal and hull, schema fields, AngleToForward, round end, beam fields, owner handle, player console commands), M2 core (economy, shop, loadouts, placement, structures, structure health, turrets, laser mines, walls, look-at and upgrades, Cores and rounds, addon), M3 (sabotage, sensor towers, teleporters, jump pads, turret branches, scrap, bounties, supply drops, landmines, dispensers, tesla coils, perks), M4 VIP tiers through admin-system's permission contract, the anticheat and bhop integration, and M5 (rocket battery, air defense, net, scout and gun drones, tank).
- Still open by choice: the HUD and tutorial screens (center text and menus stand in), the F4 particle helper (plugin-side particles work, two-control-point particles do not), the live balance pass, lifetime stats persistence, drone markers and tank crush damage.
- Unverified on Linux (no `libserver.so` here; check before any Linux deploy):
  - signatures: `CBaseEntity::TakeDamageOld`, `CTakeDamageInfo::CTakeDamageInfo`, `CCSGameRules::TerminateRound` (CS2Fixes' bytes);
  - `CNavPhysicsInterface::Nav_TraceShape` at index 5 (inferred from the ABI);
  - schema baselines copied or inferred from Windows in `schema/server.linux.json`: `CPlayer_CameraServices`, `CCSPlayerBase_CameraServices`, `CCSGameRulesProxy`, `CBeam`.
- Framework finding to follow up: `EntityOps::RemoveDelayed` (the delayed `Kill` IO event) crashes the server without a minidump; stronghold no longer calls it. Fixed 2026-09-22, see below.
- Client checklist (everything that needs a human in game):
  - Shop and menus: the center HTML shop pages and price rows, the Perks and VIP pages, the turret branch menu, `sh_shop` in the console and `bind b sh_shop`, the `!menu` Stats-tab entry, no upgrade while a menu is open.
  - Placement: the ghost visible only to the placer, green and red, E places and charges (the shop's E does not place at once), R cancels, the prompt text, the teleporter's two-step placement and refund, wall facing and scale, laser mines facing out of the wall, placement boxes that match the models (wall, dispensers, tesla coil, rocket battery, air defense, tank).
  - Structures: the turret head on its stand, turning and pitching, red and blue groups surviving upgrades; dispenser `ct_/t_` and money `level_` groups; the laser beam (visible, team colour, from the emitter, gone while unpowered); the dimmed unpowered look; destroy and hit sounds; the landmine beacon particle and blast sound; tesla coils removing grenades and repairing; the sensor tower glow for the team only; teleporter trips; jump pad feel and no fall damage.
  - Kill feed and HUD: turret, laser, landmine, rocket and tank kills (owner name and the `prop_dynamic` icon), the look-at center text line breaks, the rocket warning banner and siren, "Core under attack", the round summary and win panel, bounty and VIP clan tags on the scoreboard, and whether any tag reaches the kill feed.
  - Perks: speed feel and whether it survives weapon switches and scoping, gravity jump height, health above 100 on the HUD, Cryo's slow on a real player.
  - VIP: the page with a real VIP account (swap the seed's placeholder SteamID and `!admin_reload`), tints and beam colours, the salary line, `sh_core_set` for an admin and its refusal for others.
  - Anticheat: in observe mode, own turrets, landmines, rockets, drones and tanks while shooting elsewhere, plus teleporters, jump pads and the perks, should raise nothing.
  - Vehicles (F5): the view through the camera for drones and the tank, mouse steering while seated (if the view is pinned, switch steering to mouse deltas), WASD, jump and crouch, E in and out, the knife switch; rocket facing, exhaust trail and explosion; the air defense turning and interceptor; net and pole models; tank parts lined up, the gun on the turret, and the chase camera.
  - Assets: every model's scale and material groups on first spawn, and the Core's size and spawn-centroid position on each map (save positions with `sh_core_set`).
- Release steps for the user:
  1. Review and push the `feat/stronghold` branches (voltmod first, then `plugins/stronghold` and `plugins/anticheat`, then the root).
  2. Tag a voltmod release that contains the `feat/stronghold` commits (`/release`), so CI uploads the Conan package.
  3. From the repo root, `uv run poe build --relock` and commit `conan.lock` (this drops the `conan editable` link used during development).
  4. Upload the `stronghold` workshop addon in the Workshop Manager, then set its id as `addonId` in the plugin's `configs/settings.jsonc` (0 skips it). Updates wait for Steam moderation.
  5. Verify the Linux items above on `libserver.so` before the first `/deploy-test`, then deploy to one test server and run the client checklist.

### 2026-09-22 — Delayed removal fix

- Root cause: voltmod bound `CEntitySystem::AddEntityIOEvent` with ten parameters, an `int outputId` in the eighth slot. The engine takes nine, and the eighth is a pointer it copies from when non-null. MSVC stores an `int 0` in a stack argument slot with a 4-byte `mov dword ptr [rsp+38h]`, so the slot's upper half kept stale stack bytes. The engine read a wild pointer and crashed inside `new` + copy-construct. The input name and the variant were not the problem.
- Evidence:
  - Disassembly of `server.dll` (signature at RVA `0x1247710`): it reads exactly nine arguments. Argument 8 (`[rsp+0x98]`) is null-checked, then passed to a copy constructor at `0x14513f0` for a new 0x90-byte object. Argument 9 is copied into the event's `KeyValues3`. The input name is interned through `CUtlSymbolTableLarge::AddString` at `this+0x1ec8` (`0x514960`, MurmurHash2 seed `0x31415926`). The variant is deep-copied with `g_pMemAlloc` (`0x1492a0`: FIELD_CSTRING gets a fresh buffer and its own `CV_FREE`). `CEntityInstance::AcceptInput` (`0x126bad0`) reads only five arguments; voltmod passed seven, which was harmless but wrong.
  - Live repro on the local server, with a temporary `sh_probe_kill` command that was not committed: the old `RemoveDelayed` killed the server on the first call from RCON, and CS2 wrote no dump. A ctypes debugger attached to `cs2.exe` caught the first-chance fault and wrote one: `0xc0000005`, read of `0x7e00000000` at `server.dll+0x145147f` (`cmp dword ptr [rbp], r12d` in that copy constructor). The low half of the address is the `int 0`; the high half is stack garbage. Keeping the same temporary name and allocated `variant_t("")` but passing a full-width null in slot 8, or calling the correct nine-argument prototype, removed the prop on time with no crash, even with the name buffer overwritten right after the call.
- Fix (voltmod `474392c`): `Bindings::AddEntityIOEvent` now has the engine's nine parameters with pointer-typed trailing arguments, and `EntityOps::AddIOEvent` passes `nullptr` for both. `Bindings::AcceptInput` now has its five parameters. The wrong ownership comments (that `variant_t` borrows the string) are corrected; the SDK `variant_t(const char*)` copies its string. The public API is unchanged.
- Stronghold (`de4757c`): `Projectiles` again removes impact particles and hidden lingering shot props with `RemoveDelayed`, and drops its frame-loop linger list.
- Verified live on de_dust2, then after `changelevel de_inferno`:
  - `RemoveDelayed` and `AddIOEvent` at delays 0 and >0, and an IO event carrying a string value (`SetScale "2"`, then Kill).
  - Two Kills on one entity, `RemoveDelayed` followed by an immediate `Remove`, and `AcceptInput` / `AcceptInputFloat` before a delayed Kill.
  - Stronghold's impact particle plus a hidden prop.
  - Two soaks: 3.7 and 3.6 minutes, 37 and 33 rounds of 510 to 550 calls, about 30,000 calls in total. 200 Kills were still pending across the map change; the second soak had eight bots playing.
  - No fault reached the attached debugger. Every sampled prop was gone after its delay, and the `prop_dynamic` count stayed flat.
  - `poe build`, `poe test` (295 root, 472 voltmod) and `poe lint` pass in both repos.
- Still open: the Linux side has the same nine-argument function and the same fix applies, but `libserver.so` was not disassembled here. The rocket impact path in a real match still needs a client (it is on the checklist above).

### 2026-09-22 — VoltMod native SDK refactor

- Goal: replace hand-rolled engine code in voltmod with the hl2sdk's own types and functions, per the audit. Everything is on `feat/stronghold`; nothing is pushed, tagged or relocked.
- Lock: voltmod's `conan.lock` was stale. The bot bumped the recipes to hl2sdk `2026.09.14` and metamod `2.0.0.20260915` on 2026-09-16 without relocking. It now pins the published revisions (`hl2sdk-cs2 d6d1810b`, `metamod-source 171a73fa`). The remote has Linux binaries only, so the Windows ones were built locally with `uv run poe release build sdk`. The root lock and the editable build still resolve hl2sdk `2026.09.10`; the only header difference is `CHandle::operator=(const CBaseHandle&)` in `ehandle.h`. The release relock picks up the new pin.
- voltmod commits:
  - `bf79750` lock refresh.
  - `fb5d1dc` `fix!`: `SetRender` takes the generated `Schema::RenderMode_t`. CS2 numbers the modes 0 normal, 1 trans-alpha, 2 none. `Pawn::SetVisible` now writes `kRenderTransAlpha`; before, it wrote 3, which is out of range. GlowVision's relay now gets `rendermode 2` instead of 10. The engine stores the keyvalue unchecked, so 10 reads back as 10.
  - `3e94a4b`: `DamageHeadshot` is `DMG_HEADSHOT` (1<<19, not 1<<23), and every `Damage*` constant is static_asserted against `DMG_*`.
  - `a182693`: `Resolve` and `RawController` go through `CEntitySystem::GetEntityInstance(CEntityHandle)` and `GetEntityIdentity(CEntityIndex)`, so the hand-written chunk walk and `& 0x7FFF` are gone. `Entity::Ref`, Trace's hit ref, `Index`, `ClassName` and EmitSoundFilter's source index use `GetRefEHandle`, `GetEntityIndex` and `GetClassname`, so every ref is built the same way `Resolve` compares it.
  - `d3d0ad9`: Visibility reads the weapon and wearable handles as `CUtlVector<CEntityHandle>`, with a size assert of 24.
  - `3efe701`: ClientConVars posts through `SingleRecipientFilter` instead of the raw-mask overload. Statuses are asserted against `EQueryCvarValueStatus` and validated with `EQueryCvarValueStatus_IsValid`.
  - `1463caf` `refactor!`: the `VoltMod::IN_*` and `VoltMod::FL_*` copies are gone; the SDK's `in_buttons.h` and `const.h` names are used instead. `VoltMod::MoveType` is replaced by the generated `Schema::MoveType_t`.
  - `edd47cf` `refactor!`: `ObserverMode_t` is renamed to `ObserverMode`, so no VoltMod name shadows an SDK name. The team constants now live in one SDK-free `Entities/Teams.hpp`, which Targeting also uses; its private copy is gone. The `ObserverMode`, `HitGroup` and team values are static_asserted against `OBS_MODE_*`, `HITGROUP_*`, `TEAM_*` and `CS_TEAM_*`. The subject line is over 72 characters, and the commit was left as is rather than amended.
  - `c2747e2`: `AngleToForward` is out of line on mathlib's `AngleVectors`, and `PawnOps::ClearedDestination` uses it.
  - `84300a0`: TextMsg uses `HUD_PRINT*` directly.
  - `a03e172`: `SetVisible` packs its alpha with `Color::GetRawColor`, and `IsAlive` compares with `LIFE_ALIVE`.
  - `ea2fd6f`: the damage layout's handles are `CEntityHandle`, and its flinch hitgroup is `HitGroup_t`.
  - `34218f6`: `MaxPlayers` is asserted against `ABSOLUTE_PLAYER_LIMIT`, and all 17 `ConVarType` values against `EConVarType_*`.
  - `2afa03e`: `FindByClassName` and `FindByName` use `EntityInstanceByClassIter_t` and `EntityInstanceByNameIter_t`. The class search starts from `after` along the active list; the name search steps past `after`. The two gamedata signatures and their `Bindings` members are removed (18 functions now).
- Consumer commits:
  - stronghold `7b752d9` (render modes) and `30c6f4c` (IN_/FL_ names).
  - anticheat `51764a9` (IN_/FL_ names, move types, VoltMod's teams instead of its own copy).
  - root `df7a094` (disco render mode) and `31224b9` (admin-system cheat check and menu, bhop).
- Verified live on the local de_dust2 server with six bots, through a temporary `sh_probe` command that was not committed:
  - Before the gamedata entries were removed, the old signatures and the SDK iterators returned identical lists in identical order: 14 class queries (including `point_*`, `env_*` with 158 entities and `weapon_*`) and 7 name queries (spawned `targetname`s, the `probe_*` wildcard, a missing name, `!player`, `!self`). After the removal, the counts were the same.
  - Entity lookups:
    - `Resolve(Ref)`, `Index` and `ClassName` held for all 311 entities, and a stale ref resolved to nothing.
    - Each slot's controller sits at index slot+1, and `SlotOf` round-trips.
  - A solid trace from one bot's eye resolved `HitEntity` to the other bot's pawn.
  - Rendering:
    - `SetVisible(false, 128)` wrote mode 1 and `80ffffff`, and `SetVisible(true)` restored 0 and `ffffffff`.
    - GlowVision spawned its relays with `kRenderNone`.
  - Damage:
    - `Damage.Apply` with `DamageBullet | DamageHeadshot` arrived at the hook as `0x80002` and lowered health.
    - Across more than 120 natural bot hits, neither bit 19 nor bit 23 was set at `TakeDamageOld` entry. The damage is still the base ~35 there, before the hitgroup applies, so the headshot value rests on the SDK and the schema.
  - A turret placed through `Structures::Build` next to an enemy bot killed it (frags 1), and the hook saw `prop_dynamic` as the inflictor.
  - `poe build`, `poe test` (295 root, 472 voltmod) and `poe lint` pass in both repos, and `modgraph` holds.
- Kept, and why:
  - `ConVar<T>::Get` stays on the stored `CVValue_t*`. `GetAs<T>` would mean storing a `ConVarRefAbstract`, and `SetRaw` pokes the same storage. `Set` stays on the cfg line.
  - The cstrike15 usermessages proto change is skipped; the reflection route is deliberate.
  - `Damage*`, `ObserverMode`, `HitGroup` and the teams stay VoltMod definitions with static_asserts, because `shareddefs.h` is too heavy for public headers and Teams must stay SDK-free.
  - `ColorOpaqueWhite` and `ColorInvisible` stay `uint32_t` constants, because the SDK's `Color` is not constexpr.
  - The disco effect and stronghold's hidden or tinted parts moved from the old `TransTexture` to `kRenderTransAlpha`. Parts hidden with alpha 0 could use `kRenderNone` instead.
- Needs a client:
  - The look of the alpha and the tints.
  - The glow outline and the invisible relay.
  - Visibility filtering: bots receive no transmit.
  - ClientConVars: a bot has no net channel, so `Query` returned false as designed.
- Linux, unverified: the SDK iterators and `GetEntityIdentity` read `CEntitySystem` and `CEntityIdentity` through the SDK's struct layout (`m_EntityList`, `m_entityNames`, `m_Symbols`). This was checked on Windows only; check it on `libserver.so` with the other Linux items above.
- Noticed and left alone: the `FindByClassName` loop in `docs/sdk/entities.md` assigns an `Entity`, which does not compile, because wrappers are not assignable.
