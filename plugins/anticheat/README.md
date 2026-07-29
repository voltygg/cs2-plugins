# Anticheat

Server-side cheat detection for CS2 community servers: aim analysis over correlated shots,
plus client-integrity checks. No client component, no detours, game thread only.

It reads two feeds the kit already provides - the `RunCommand` movement hook (one decoded
usercmd per player per tick) and game events (`weapon_fire`, `bullet_impact`, `player_hurt`,
`player_death`, `player_spawn`, settings changes).

Detection algorithms are reimplementations of [CS2AC](https://github.com/karola3vax/CS2AC)
(karola3vax, AGPL-3.0); the approach and the tuning are theirs, the code is written against
[CS2Kit](../../vendor/cs2-kit).

> **Start in `observe` mode.** Nothing is done to any player until you say so - see
> [Rollout](#rollout).

## Detections

Seven modules, each with its own kill switch under `anticheat.detections`. Thresholds are
compiled in and deliberately not operator-tunable: if a module misfires, turn it off rather
than loosening it.

| Module | Catches | Reports after |
| --- | --- | --- |
| `aimbot` | Aim snapping onto a target on the command that dealt damage | 4 incidents / 10 min |
| `aimlock` | Aim held inside a *moving* target's width for 1.5 s | 3 episodes / 10 min |
| `antiaim` | Impossible or fabricated angles, spinning, jitter binds | score 100; spin and jitter fire at once |
| `silentaim` | Bullet landing far from where the player visibly aimed | 12 points / 10 min |
| `dll_injection` | Client subscribed to game events no stock HUD wants | first hit |
| `invalid_cvar` | Client convars outside their allowed values | first hit, then latched |
| `namechanger` | Visible name-change spam | 5 changes / 60 s |

Every module is gated by: the master `enabled` switch, `sv_cheats` being off (unless
`allowSvCheatsTesting`), and the player being a connected non-bot human. A map change,
`anticheat_reload`, an `sv_cheats` flip or an `mp_teammates_are_enemies` change drops all
accumulated evidence.

### How each one decides

**`aimbot`** judges only commands that actually damaged an enemy. It walks back along
strictly adjacent commands (any gap ends the chain) for up to 32 ticks looking for
convergence - a jump over 10° that lands inside 20% of its previous aim error, or over 5°
inside 10% - plus a one-command snap-return rule. Needs 100+ units of range, opponents, and
both parties clear of the 5 s teleport grace.

**`aimlock`** wants 1.5 s of aim inside the target's own angular width for 95% of samples
*while the target moves* at least 48 units of angular travel - a still target proves nothing.
Two players under the crosshair means no episode. Since clients aim at an interpolated past,
it carries a set of lag hypotheses (round trip + `cl_interp_ratio`, ±2 ticks) and needs one
to survive the whole episode; unusable network readings mean no estimate and so never
evidence. Dying drops the episode in progress but not episodes already counted.

**`antiaim`** runs one decaying score: pitch past 89.01° or roll past 50.01° adds 2, a
structurally impossible command adds 1, a base view 120°+ from the angles the command claims
it fired along adds 1 (at most every 4 commands - a fast flick can do it once), a one-command
attack return adds 5. Spin and jitter are separate and fire immediately: spin needs sustained
rate *and* 0.85 direction consistency (that is what separates a spinbot from a player
flicking), jitter needs an exactly repeating yaw pattern - so a legitimate 180 bind, which
looks identical for a moment, never qualifies.

**`silentaim`** compares the visible eye angles at `weapon_fire` against where the bullet
landed, scoring only shots that both hurt someone and reported an impact. The ceiling is per
weapon class (2.5° snipers, 4.3° pistols, 12.5° rifles, 22.5° SMGs - spray and movement
legitimately throw SMG bullets wide). Headshots and wallbangs score extra; airborne shots
score less.

**`dll_injection`** looks for client event subscriptions no HUD ever wants, read server-side
without touching the client. First scan is 10 s after full connect (the listener does not
exist the instant a player joins), then every 120 s.

**`invalid_cvar`** polls the client and reads userinfo. **Silence is never evidence** - a
client is under no obligation to answer. A reply that *refuses* a value counts only for
cheat-protected convars, only after three in a row, and only as a kick, so a convar a future
CS2 update removes cannot kick a whole server. Each (player, convar) latches: it reports once
and re-arms only after reading valid.

**`namechanger`** counts distinct visible names. The baseline is kept current even while
detections are gated off, so a change is never measured against a stale name.

## Response

Every detection is logged and sent to the webhook regardless of mode. `anticheat.mode`
decides what else happens:

| Mode | Behaviour |
| --- | --- |
| `observe` *(default)* | Log and webhook only. The dry run. |
| `alert` | Also pings admins with the Ban flag, once per (player, detection) per 30 s. |
| `ban` | Also punishes: kick-only findings kick, the rest ban. |

- **Kick-only findings never ban**, even in `ban` mode - the ones whose false-positive cost
  must stay recoverable: `m_yaw` out of range, `fps_max` below 64, and a refused
  cheat-protected convar.
- **Punishment only escalates.** A later detection cannot demote an issued ban to a kick, and
  a repeat cannot double-punish. Cleared on disconnect, map change and reload.
- **Whitelisted SteamID64s** are logged and webhooked but never alerted or punished. So is a
  player whose SteamID has not resolved yet.
- **Bans go through admin-system** (`as_ac_ban` / `as_ac_alert`) so persistence, kicking and
  broadcast stay in one place. Without admin-system loaded, banning degrades to logging;
  kicks still work.
- **Discord** is optional and dormant while `webhook.url` is empty. One embed per (player,
  detection) per minute; failures log once and are not retried.

## Configuration

Two files, split by who owns them.

### `configs/settings.jsonc` - operator knobs

Rendered per server from
[`deploy/templates/…/settings.jsonc`](../../deploy/templates/plugins/anticheat/settings.jsonc).

| Key | Default | Meaning |
| --- | --- | --- |
| `enabled` | `true` | Master switch. |
| `mode` | `"observe"` | `observe`, `alert`, or `ban`. |
| `banDurationSec` | `0` | Auto-ban length; `0` = permanent. |
| `whitelistSteamIds` | `[]` | Reported but never punished. |
| `allowSvCheatsTesting` | `false` | Keep detecting while `sv_cheats` is on. Test boxes only. |
| `detections.*` | `true` | Per-module kill switches (see the table above). |
| `webhook.url` | `""` | Discord webhook; empty disables reporting. |
| `debug.simulator` | `false` | Arm the cheat simulator. Dev boxes only. |

### `configs/detections.jsonc` - what the checks compare against

The blacklisted event names and the convar rules. These track *Valve's* content, not this
plugin's logic, so a CS2 update that renames a convar or adds a HUD event is answered with an
edit here and `anticheat_reload` - no rebuild, no redeploy. It ships identically everywhere,
so it has no deploy template.

A rule that fails to validate is dropped with a warning and the rest still loads. A file that
fails to parse at all leaves the tables already in memory in force, so a typo cannot silently
disarm a module.

Each rule is `{ name, tier, constraint, … }`:

| Field | Values |
| --- | --- |
| `tier` | `queried` (asked over the network) or `userinfo` (read from userinfo). One tier per convar - both share a latch. |
| `constraint` | `equals` / `max` / `range` / `minOrZero` / `off` / `on` |
| `value`, `max` | Bounds for the constraint. |
| `cheatProtected` | Defer until a disabled `sv_cheats` has reached the client. |
| `kickOnly` | Cap the punishment at a kick. |

## Commands

| Command | Description |
| --- | --- |
| `anticheat_status` | Snapshot of gates, module toggles and dependency health, plus a line per human player (punishment level, per-module counters, latched convars, pending queries). Also published as the `anticheat` section of the kit's status service. |
| `anticheat_reload` | Re-read both config files **and drop all accumulated evidence**, including the punishment latch. |
| `anticheat_dumpcmd <slot> [ticks=64]` | Log raw usercmds for one slot. The tool for checking a suspicion against real traffic. |

### Cheat simulator

> ⚠️ **These rewrite live decoded input.** Gameplay is unaffected - the engine still runs the
> player's real command - but that player's readings become fiction while a sim is running.
> **Never arm this on a server people are playing on.**

Gated by `anticheat.debug.simulator`, and registered only at load, so enabling it needs a
plugin reload rather than `anticheat_reload`. Each sim runs for 10 s. box-a's deploy template
arms it; no other server's should.

| Command | Simulates |
| --- | --- |
| `anticheat_sim_spin <target> [degPerSec=720]` | Spinbot. |
| `anticheat_sim_jitter <target> [stepDeg=20]` | Repeating jitter bind. |
| `anticheat_sim_badangles <target> [pitch=89.5]` | Impossible pitch and roll. |
| `anticheat_sim_aimlock <target>` | Locks onto the nearest opponent. |
| `anticheat_sim_mismatch <target> [deg=130]` | Fired angles diverging from the visible view. |
| `anticheat_sim_off [target]` | Stop one sim, or all when omitted. |

`<target>` is a slot or a SteamID64. There is no silentaim sim: it scores where the bullet
actually landed, and the filter only edits the decoded view.

## Rollout

Deployment is currently **held** - `anticheat` is commented out in
[`deploy/inventory.yml`](../../deploy/inventory.yml) in both the plugin list and box-a's
server entry. Uncomment both to ship it.

1. **Observe.** Deploy to box-a in `observe` with `webhook.url` set. Soak through real
   traffic - a week of populated hours, not an empty server.
2. **Read the misses.** For anything that looks wrong, `anticheat_dumpcmd` the slot and
   compare against the evidence string. A module producing false positives gets switched off.
3. **Alert.** Once the observe stream is clean, flip to `alert` and let admins be the funnel
   for a second soak.
4. **Ban, one module at a time.** Flip to `ban` with only the modules you trust. Start with
   `invalidCvar` (worst case is a recoverable kick), then `namechanger` and `dllInjection`,
   and add the aim modules last - `silentAim`, `antiAim`, then `aimbot` and `aimlock` - with
   a soak between each.

## Maintenance

Six gamedata values drift with CS2 updates. Re-verify them against SwiftlyS2 / CS2Fixes /
CS2AC after **every** game update - a quiet detection looks exactly like a clean server.

| Entry | If it drifts |
| --- | --- |
| `RunCommand` | **Crash** on the first movement tick. |
| `UserCmdPB` | Missing: aim modules go silent. Stale: garbage angles that look like plausible data. |
| `UserCmdNumber` | Command numbers all collapse to 0, so no convergence chain links: `aimbot` and half of `antiaim` go **permanently silent**. No status field, no log line - check this one explicitly. |
| `Teleport` | The post-teleport grace stops suppressing the discontinuity: false positives rather than silence. |
| `ProcessRespondCvarValue` | Sanity-bounded at init, so it degrades instead of hooking an unrelated function. |
| `ServerSideClientSlot` | Sanity-bounded too; unchecked it would credit one client's answer to another player. |

`anticheat_status` surfaces the ones it can: `teleportTracker`, and `"clientCvars":
"degraded"` when the two `ClientCvars` offsets fail (the network poll stops and
`invalid_cvar` falls back to userinfo values only).

## Architecture

For contributors. Every detection is a **core**: an SDK-free class over plain sample structs
with its tuned constants `constexpr` in its own source file. Cores unit-test directly - the
whole detection layer compiles into `anticheat-tests` without linking the SDK. Thin adapters
convert engine state into samples and hand verdicts to the response funnel.

```text
plugins/anticheat/
  src/
    Core/          Samples, Geometry (great-circle angle math), WeaponClass, Finding
    Correlation/   ShotCorrelatorCore (SDK-free) + ShotCorrelator (the single engine feed)
    Detectors/     *Core.cpp = SDK-free rules; *Detector.cpp = engine adapter
    Response/      FunnelPolicy (pure decision logic), ResponseManager, DiscordReporter
    Simulator/     CheatSimulator (dev-only input synthesis)
  configs/         settings.jsonc (per server) + detections.jsonc (shared)
  tests/
```

The backbone is the **ShotCorrelator**, which joins a usercmd to the shot it fired and to the
events that shot produced, so every aim module reasons about "this exact command fired this
exact bullet at this exact world state". Matching is deliberately strict: a `weapon_fire`
binds to a command only when *exactly one* candidate sits in the window, and an ambiguous
window burns all its candidates so a later event cannot claim one arbitrarily. It also keeps
128 ticks of per-slot position frames. The firing angle always comes from the command's input
history - an index the transport cap dropped means *absent*, never clamped back into range,
because clamping reads another shot's angles.
