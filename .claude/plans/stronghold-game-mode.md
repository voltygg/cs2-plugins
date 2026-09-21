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

- [ ] Create the `stronghold` addon in the Workshop Tools (`<CS2>/content/csgo_addons/stronghold/`).
- [ ] Copy the needed sources from `references/wwdm/` under our own paths: `models/stronghold/...`, `materials/stronghold/...`, `particles/stronghold/...`, `sounds/stronghold/...`, `soundevents/soundevents_stronghold.vsndevts` with names like `Stronghold.Build.Upgrade`. Rename every file and internal reference that carries a foreign prefix.
- [ ] Compile with the local `resourcecompiler`; fix errors; leave out and note anything that still fails.
- [ ] Case-insensitive grep over the addon tree and `plugins/stronghold` for the foreign prefixes comes back empty.
- [ ] `plugins/stronghold/docs/assets.md`: path, source ("reference pack" or own), licence status.

Ownership: the reference assets belong to their original authors. Treat them as development stand-ins; the `assets.md` table is the list to replace or clear before the addon is published publicly. The item list is config-driven, so swapping a model is a one-line change.

## 5. Milestone 1 — framework work (voltmod)

Each item: gamedata entry + `Bindings` member + public API + doc page + doctest where SDK-free. Windows and Linux signatures both.

### F1 Damage

- [ ] Bind `CBaseEntity::TakeDamageOld` and describe `CTakeDamageInfo` (attacker, inflictor, ability handles; damage; damage type; hit group; position). Accessor struct, no raw offsets in plugins.
- [ ] `runtime.Hooks.Damage.Before(handler)`: handler sees victim entity + mutable damage info and returns allow / block. Fires for **every** entity, not just players.
- [ ] `Damage::Apply(victim, spec)` with `spec{Attacker, Inflictor, Amount, Type}`: builds a `CTakeDamageInfo` and calls the engine, so death, kill feed, `player_death` and stats are the engine's own.
- [ ] Spike before polishing: one `prop_dynamic` that (a) kills a bot through `Apply` with the owner credited, (b) reports bullet damage through `Before`. If bullets never reach `TakeDamageOld` for `prop_dynamic`, try `m_bTakesDamage`/`m_takedamage`, then `prop_physics_override` with motion disabled. Record the winning recipe in the doc page.

### F2 Trace

- [ ] Add `HitEntity` (as `EntityRef`/handle) and `Normal` to `TraceHit`.
- [ ] Add a hull trace (`Trace::Hull(from, to, mins, maxs, options)`). If the nav trace cannot sweep a box, bind the physics query path CS2Fixes/SwiftlyS2 use (`TraceShape`).

### F3 Schema manifest

- [ ] Add `m_flGravityScale`, `m_flMaxspeed` (movement services), `m_bTakesDamage`, `m_iMaxHealth`, and whatever F1's spike needs; `voltmod schemagen`; expose `Pawn::SetGravityScale`, `Pawn::SetMaxSpeed`, `Entity::SetMaxHealth`.
- [ ] Confirm the speed perk survives weapon switches (the engine recomputes max speed per weapon). If it does not, reapply in `Movement.After`.

### F4 Effects helpers (small)

- [ ] Prove `info_particle_system` (`effect_name`, `start_active`, `Start`/`Stop`/`DestroyImmediately`) and `env_beam` or a two-control-point particle for the laser. If a thin `Effects::PlayParticle(path, origin, angles, lifetime)` falls out, add it to voltmod; otherwise keep it in the plugin.

### F5 Vehicles prerequisites (do in M5, listed here for completeness)

- [ ] Camera: prototype in order — (1) observer mode targeting the vehicle entity (`Pawn::SetObserverMode`, `CPlayer_ObserverServices::SetObserverTarget`), (2) `point_viewcontrol`-style camera entity if CS2 still honours it, (3) hiding the pawn and moving it as the vehicle. Pick the first that gives a stable first/third-person view with the player's pawn safe and parked.
- [ ] Usercmd writeback or a "suppress movement" flag in the Movement hook, if `MoveType::None` + reading input proves insufficient.

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

- [ ] **Mode rules** — `stronghold.cfg`: instant respawn for both teams, no buy zone/time, game cash awards off, max money raised, no round end on elimination, long round timer, warmup off. Apply on map start.
- [ ] **Wallet** — start balance, kill reward (income perk multiplier), structure-destroy reward, upgrader bonus per structure frag, `CanAfford`/`Charge`/`Pay`. Mirrors into `Controller::SetMoney`. Pure logic unit-tested.
- [ ] **Config** — `Settings` with an item list: id, kind, title key, price, base limit, per-level `{health, damage, range, fireInterval, upgradePrice}`, model paths per part, sound names. Reloadable.
- [ ] **Shop screen** — one `ForPlayer` screen, three tab panels toggled by class, static card ids (`shop_item_turret_buy` ...). Per card: price, owned/limit, state class (`can-buy` / `no-money` / `limit` / `need-vip`). Balance label. Opens on `!shop`, `sh_shop`, main-menu section, and a key: test whether the client sends `drop` (G) or `buymenu` (B) as hookable commands in this mode; bind whichever works and document the fallback `bind`. `ShowCursor` on open, off on close/death/disconnect. Keep the name count under the budget; run `uv run poe lint`.
- [ ] **Loadouts** — free weapon set applied now and on each spawn; armor and grenades as one-off purchases.
- [ ] **Placement mode** — enter on purchase click (shop closes). Ghost = same model parts, `ShowOnlyTo` the placer, translucent green/red via `SetRender`. Each frame: trace from eye along view to max distance, snap to hit point, orient by surface normal (floor items upright and yaw = player yaw; laser mine and wall-mounted items align to the wall normal). Validity: surface slope per item, hull trace clear of world/players/structures, minimum distance from spawn points and other structures, item-specific checks. E places and charges; R, weapon fire, death or shop reopen cancels. Validity rules unit-tested on plain vectors.
- [ ] **StructureRegistry** — owns all live structures: owner slot + SteamID, team, kind, level, health, frag count, list of upgraders, entity refs for parts, powered flag. Enforces per-owner limits. Owner death → unpowered until respawn (dim via `SetRender`, behaviours skip unpowered structures); removed on disconnect, team change, round/map end. Lookup by `EntityRef` for the damage hook and look-at.
- [ ] **Cores and round flow** — spawn one Core per team at the configured position, large health, damage accepted only from attackers within the radius, "under attack" alert with a cooldown, win on destruction or by health at timeout, end-of-round summary screen, wipe structures, carry over money by the configured share. `sh_core_set <team>` writes the map's position file. Round outcome logic unit-tested.
- [ ] **Structure health** — in `Damage.Before`: victim is a registered part → subtract from the structure's health (friendly fire ignored), block engine damage, pay the destroyer at zero, play break effect, remove. Hit feedback sound.
- [ ] **Turret** — states: building (short delay) → idle sweep → tracking → firing. Target selection at ~10 Hz, staggered across turrets: nearest living enemy in range with `Trace.Clear` from the muzzle, ignoring own parts. Yaw part and pitch part rotate toward the target at a capped turn rate every frame. Fires on an interval through `Damage::Apply` (attacker = owner pawn, inflictor = turret), tracer/muzzle particle, sound. Levels swap the head model and stats. Targeting math (lead-free aim angles, turn-rate clamp, range/FOV test) unit-tested.
- [ ] **Laser mine** — placed on a wall; beam endpoint from a trace along the normal; each tick test enemies against the segment (closest-point distance, cheap) and kill through `Damage::Apply`. Team-colored beam. Breaks when shot.
- [ ] **Wall** — static solid prop with health. Nothing else.
- [ ] **Look-at panel and upgrade** — per player at ~10 Hz: trace with `HitEntity`, resolve to a structure, fill the HUD panel (`SetText`/`SetHidden`). E edge while looking at an allied upgradable structure within reach charges the presser, levels it up, records them as an upgrader. "Not enough money: need $1200" toast.
- [ ] **HUD** — shared-style HUD done as a per-player screen: prompts, toasts ("+$800"), rocket warning banner slot for later.
- [ ] **Tutorial** — first-join screen with three cards and a confirm button; remembered for the map in v1.
- [ ] **Precache and addon** — `Precache.Add` for every configured model/particle/soundevent file at load; `Addons.Require(3801580041)`. Center-text fallback while a client is still downloading.
- [ ] **Translations** — English and Russian for every player-facing string.
- [ ] **Other plugins** — make sure `anticheat` does not flag speed/gravity perks or turret kills (no attacker view angles), and that `bhop` is not loaded on these servers or tolerates modified max speed.

Exit criteria: on a local server with bots, buy → place → turret kills with kill-feed credit → enemy destroys it for a reward → owner death powers the base down and respawn brings it back → a destroyed Core ends the round; 32 bots with 60+ structures holds tick rate (profile the targeting loop).

## 7. Milestone 3 — signature features, remaining structures, perks

Signature features first (section 2a) — they are what players will not find elsewhere:

- [ ] Sabotage: hold-E progress bar on the HUD, behind-the-structure test (dot product against its facing), temporary team flip, self-destruct timer, owner warning, cancel on damage to the saboteur. Turret front-cone rule for crouching enemies.
- [ ] Sensor tower: per-team glow of enemies in range using the `GlowVision` recipe; range grows with level.
- [ ] Teleporter pair: two placements in one purchase, recharge per level, telefrag protection (exit must be clear — hull trace), team-colored particle.
- [ ] Jump pad: launch velocity along placement yaw, per-player cooldown, no fall damage for the landing.
- [ ] Turret specialisation at level 3 (Gatling / Marksman / Cryo) — stats per branch in config; Cryo slow via the max-speed setter with a timed restore.
- [ ] Scrap pickups (prop + proximity check + despawn timer), bounty tracking and payout, trailing-team income boost.
- [ ] Supply drops: schedule, drop point choice, falling crate, beacon particle and siren, hold-E claim, random level-2 item placed into the claimer's placement mode for free.

Then the borrowed basics:

- [ ] Landmine (proximity trigger by distance check, explosion via `env_explosion` or radius `Damage::Apply`, breaks when shot).
- [ ] Health dispenser and money dispenser (aura tick, transfer particle, level screens via material group/skin).
- [ ] Tesla coil (repairs allied structures in range; removes enemy grenade projectiles in range — find by classname each tick; arc particles).
- [ ] Perks: speed, gravity, +25 HP (until death), income, regen. Decide and document which perks reset on death.
- [ ] Balance pass on a live test server; all numbers in config.

## 8. Milestone 4 — VIP tiers and persistence

- [ ] Tiers `basic / lite / medium / ultra / extreme` as permission strings (`stronghold.vip.<tier>`) resolved through `runtime.Policy` so admin-system groups can grant them. No new tables unless expiry dates are needed; if they are, a `stronghold` DB with its own migrations (test data in seed files, not migrations).
- [ ] Tier effects from config. Default set is the fair one from section 2a: structure skins, beam colors, kill-feed tag, loadout presets, faster placement, modest salary. The reference's power perks (higher limits, timed free HP/armor/money, discounts) stay available as config options, off by default.
- [ ] VIP tab: shows current tier, cooldown timers, and what each tier adds. Movement extras (extra air jumps, parachute, grapple) only if wanted — each is its own small task.
- [ ] Optional persistence of lifetime stats (structures built, structure frags) for a leaderboard.

## 9. Milestone 5 — air and armor (optional)

Optional and last: this is the part copied most directly from the reference and the most expensive. Gate: players ask for it **and** the F5 camera prototype works. Otherwise M2–M4 are the product.

- [ ] Rocket battery: sky check (upward trace must reach sky/no hit), target picked by looking at a ground point, warning banner + siren to everyone, salvo of rocket props moved per frame on ballistic arcs with exhaust particles, radius damage on impact, then the battery removes itself.
- [ ] Air defense: targets enemy rockets and drones in range, fires interceptor props, destroys the target on proximity.
- [ ] Net: overhead solid that drones collide with.
- [ ] Scout drone and gun drone: pilot's pawn parked and protected or vulnerable (decide), input read from `PlayerInput`, drone moved by velocity with simple collision traces, battery/lifetime, markers on spotted enemies, strike / gun fire through `Damage::Apply`. Exit on E or destruction.
- [ ] Tank: hull + turret + gun + tracks as parented parts, ground-following by downward traces, turret follows view yaw, cannon shell as a moved prop with radius damage, heavy health, crush damage optional.

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
