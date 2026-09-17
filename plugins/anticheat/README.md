# Anticheat

A server-side CS2 anticheat built around movement commands, shot correlation,
sight lines, game events, and client-integrity checks. It has no client
component, and its runtime analysis stays on the game thread.

The detectors adapt techniques from
[CS2AC](https://github.com/karola3vax/CS2AC) to the
[VoltMod framework](https://github.com/voltygg/voltmod).

> Start in `observe` mode. Validate real server traffic before enabling alerts
> or punishments.

## Data sources

The plugin consumes:

- Decoded `RunCommand` input, one user command per player per tick, with the
  mouse counts it carries and the pawn's predicted recoil punch, scope state
  and burst counter read as it arrives.
- `weapon_fire`, `bullet_impact`, `player_hurt`, `player_death`, and
  `player_spawn` events. The pawn names the command that fired, so a fire event
  binds to its exact command.
- Sight lines traced through the framework's `Hooks.Trace` service: each tick,
  from every checked player to the enemy nearest their crosshair, and on a hit
  from the shooter's teammates to the victim.
- Relevant game-setting changes.
- Client convar responses and userinfo values.

## Detectors

Each detector has a kill switch under `anticheat.detections`. Thresholds are
compiled into the plugin.

| Detector | Looks for | Reports after |
| --- | --- | --- |
| `aimbot` | Aim snapping onto the player damaged by that command | 4 incidents in 10 minutes |
| `aimlock` | Aim held inside a moving target's angular width | 3 episodes in 10 minutes |
| `antiaim` | Impossible angles, fabricated fire angles, spin, and jitter | Score 100; spin and jitter report immediately |
| `silentaim` | Impacts far from the visible aim direction | 12 points in 10 minutes |
| `triggerbot` | Hits within 47-94 ms of an enemy walking into a resting crosshair | 8 points in 10 minutes |
| `recoil` | Sprays whose view cancels the actual recoil punch | 3 sprays in 10 minutes |
| `aim_assist` | View turns the mouse counts cannot explain that land on an enemy | 6 turns in 10 minutes |
| `wallhack` | Following, pre-aiming and shooting enemies through cover | 6 points in 10 minutes |
| `dll_injection` | Client event subscriptions unused by the stock HUD | First match |
| `invalid_cvar` | Client convars outside allowed values | First confirmed invalid value |
| `namechanger` | Repeated name or clan tag changes | 5 changes in 60 seconds |

All detectors require the master switch, a connected human player, and
`sv_cheats` off unless `allowSvCheatsTesting` is enabled. Map changes, reloads,
`sv_cheats` changes, and `mp_teammates_are_enemies` changes clear accumulated
evidence.

### Aimbot

Aimbot only evaluates commands that damaged an opponent. It scans up to 32
strictly adjacent commands and looks for rapid convergence: more than 10
degrees landing within 20% of the previous aim error, or more than 5 degrees
landing within 10%. It also detects a one-command snap and return. The shot
must be at least 100 units away, and both players must be outside the
five-second teleport grace period.

### Aimlock

Aimlock needs 1.5 seconds with at least 95% of samples inside one target's
angular width while that target travels at least 48 angular units. A stationary
target or two targets under the crosshair produces no episode.

To account for interpolation, it evaluates lag hypotheses derived from
round-trip time and `cl_interp_ratio`, including plus or minus two ticks. If
network timing is unusable, it records no evidence. Death ends the current
episode but keeps completed episodes.

### Antiaim

Antiaim uses a decaying score:

- Pitch beyond 89.01 degrees or roll beyond 50.01 degrees adds 2.
- A structurally impossible command adds 1.
- A base view at least 120 degrees from the command's fire direction adds 1,
  at most once every four commands.
- A one-command attack return adds 5.

Spin and jitter walk the client's own command sequence, so server-side batching
or a lagging connection does not break an episode. Spin reports after sustained
one-direction rotation with at least 0.85 directional consistency: 10 seconds
above 320 degrees a second, 6 above 1000, or 3 above 2200. Jitter requires an
exactly repeating yaw pattern for 5 seconds, so a legitimate 180-degree bind does
not report.

### Silent aim

Silent aim compares visible eye angles at `weapon_fire` with the reported
impact, and only scores shots that both hit a player and produced an impact.
Allowed error depends on weapon class:

| Weapon class | Maximum error |
| --- | --- |
| Sniper | 2.5 degrees |
| Pistol | 4.3 degrees |
| Rifle | 12.5 degrees |
| SMG | 22.5 degrees |

Headshots and wallbangs add weight; airborne shots add less.

### Triggerbot

Triggerbot measures reaction time the way a server can: each tick, for every
enemy and every lag hypothesis, it records when the crosshair came to rest on
that enemy's hull in the world the client was looking at. When a shot hurts
that enemy and it is the first shot of a burst (no fire in the previous 10
ticks), the reaction is the fire tick minus the *earliest* of those rests, so a
wrong lag guess can only make the reaction look slower. The crosshair must have
moved less than 2 degrees around the crossing: the enemy walked in, the shooter
did not flick. A reaction of 3 ticks or less scores 2, 4 to 6 ticks scores 1,
and 8 points in ten minutes report. Human reactions land at 10 ticks and more.

### Recoil control

Recoil control compares the view with the recoil punch across a spray. The
punch the client predicted for each command is read from the pawn as the
command arrives, so the punch a shot left behind is the one on the command
after it. Over a spray of at least 8 shots on one weapon with no gap above 16
ticks, the view change between consecutive shots is fitted against the punch
change with one factor, once for the same command and once for the command
after. A fit with a factor between 0.5 and 2.5, at least 3 degrees of punch,
and a residual of 0.35 degrees RMS or less per shot marks the spray; three
marked sprays in ten minutes report. The pattern is public, but the residual
requires cancelling the punch the engine actually produced, tick by tick.

### Aim assist

Aim assist learns each player's degrees per mouse count from their own
turns: the median of the last 32 ratios between the yaw change and the counts
the command carried, accepted once 16 samples agree within 25%. A client whose
counts never agree with its turns (a controller, or a client that does not fill
them) is never calibrated and never judged. Once calibrated, a command whose
turn differs from the counts by more than four counts' worth and more than one
degree is unexplained. It counts only when it moved the aim onto an enemy: from
outside to within 3 degrees, closing at least half of the unexplained turn.
Keyboard turning and scoped commands are skipped. Six such turns in ten
minutes report. An aimbot that moves the real mouse instead of the view escapes
this rule and is left to the others.

### Wallhack

Wallhack evidence comes from sight lines. Every tick the plugin traces from
each checked player's eyes to the head, chest and feet of the enemy nearest
their crosshair, ignoring both pawns, against world geometry and line-of-sight
blockers only (windows and clips do not hide anyone). Three rules score into
one window, and 6 points in ten minutes report:

- **Following through cover** (2 points). The aim stays within twice the hidden
  enemy's angular width (at least 3 degrees) for 64 ticks while the enemy's
  bearing moves at least 6 degrees, and the aim turns the same way for at least
  60% of that. A crosshair resting where a hidden enemy happens to pass does not
  turn, so it does not count.
- **Following into view** (2 points). An episode of at least 24 ticks ends
  because the enemy stepped into view, and a hit on that enemy lands within 16
  ticks.
- **Shooting the unseen** (2 points, 3 for a headshot). A hit through cover on an
  enemy who was hidden from the shooter at the fire tick, whom no teammate could
  see (recent frames, then a trace per teammate), who had not fired in the last
  3 seconds and was moving slower than walking speed over the previous 16 ticks.

Without a usable sight line (`sightLines` in `anticheat_status`) the module is
inert: no trace, no evidence.

### Client integrity and names

`dll_injection` checks for unusual client event subscriptions 10 seconds after
full connection, then every 120 seconds.

`invalid_cvar` treats silence as no evidence. A refused cheat-protected value
counts only after three consecutive refusals and is always kick-only. Every
player/convar pair reports once and re-arms only after a valid result.

`namechanger` tracks the scoreboard name and the clan tag together. It reads the
controller eight times a second as well as on settings changes, because a tag pushed
by a cheat does not always raise one. Its baseline stays current while detection
is gated off, so a name recorded during that gap cannot report later. After a
finding the slot is quiet for a minute, so an animated tag is one report rather
than one a second.

### What it cannot see

- **No-spread and no-recoil.** CS2 computes bullet spread and recoil on the
  server from its own seed; a client that removes them only changes what its
  owner sees. There is nothing to detect and nothing to gain.
- **Fake angles inside the legal range.** A pitch of exactly 89 or a yaw that
  only moves when the player fires is indistinguishable from real input on its
  own; the attack-return and base-versus-fire-angle rules catch the shots it
  produces, not the pose.
- **Information without action.** A wallhack that only informs the player,
  who then plays as if they had not seen, leaves no trace in the input. The
  rules above catch what the information is used for: following, pre-aiming
  and shooting through cover. Hiding enemies from clients that cannot see them
  (server-side occlusion) would blind legitimate players to peeks and sounds,
  and is deliberately not attempted.
- **Aim that moves the mouse.** A cheat driving the real cursor produces
  consistent mouse counts. Its snaps, tracking, reaction times and recoil
  cancelling still fall under the other rules.

## Verifying on a headless server

`anticheat_status` prints the module state and one line per checked player over
the console or RCON. With `debug.includeBots` and `debug.simulator` on, bots are
checked like humans and `anticheat_sim_spin`, `anticheat_sim_jitter`,
`anticheat_sim_badangles`, `anticheat_sim_names`, `anticheat_sim_aimlock`,
`anticheat_sim_nomouse` and `anticheat_sim_mismatch` drive a slot through each
detector for ten seconds; a
finding then appears in the log as `[AC] <DETECTOR> on <name>`. Bots run no
input history, so their `cmds=` counter stays at zero. Keep both switches off in
production.

## Responses

Every finding is logged and sent to the configured webhook. Mode controls any
additional action:

| Mode | Behavior |
| --- | --- |
| `observe` | Log and webhook only |
| `alert` | Also notify admins with ban access, rate-limited per finding |
| `ban` | Kick kick-only findings and ban other findings |

Safety rules:

- `m_yaw` outside its range, `fps_max` below 64, and refused
  cheat-protected convars remain kick-only.
- Punishment only escalates. Repeated findings cannot replace a ban with a kick
  or punish the same player twice.
- Whitelisted players and players without a resolved SteamID are logged and
  webhooked but not alerted or punished.
- Bans use the admin system's `Contracts::IAdminActions` interface. If the
  admin system is unavailable, bans degrade to logging; kicks still work.
- An empty `webhook.url` disables Discord. Reports are limited to one embed per
  player and detector per minute; failed sends are logged and not retried.

Punishment state clears on disconnect, map change, and reload.

## Configuration

### Operator settings

[`configs/settings.jsonc`](configs/settings.jsonc) ships with the plugin. A
deployment overrides individual keys under `plugins.anticheat.settings` in
[`deploy/inventory.yml`](../../deploy/inventory.yml).

| Setting | Default | Purpose |
| --- | --- | --- |
| `enabled` | `true` | Master switch |
| `mode` | `observe` | Select `observe`, `alert`, or `ban` |
| `banDurationSec` | `0` | Automatic ban length; `0` is permanent |
| `whitelistSteamIds` | `[]` | SteamID64s that are observed but not punished |
| `allowSvCheatsTesting` | `false` | Allow detection while `sv_cheats` is on |
| `detections.*` | `true` | Per-detector switches |
| `webhook.url` | empty | Discord webhook |
| `debug.simulator` | `false` | Register development simulator commands |

### Detection data

[`configs/detections.jsonc`](configs/detections.jsonc) contains blacklisted
events and convar rules. It is shared by all servers and can be reloaded without
rebuilding.

Parsing is strict. Unknown keys, constraints, tiers, or numeric rules without a
bound reject the complete load, and the active tables stay unchanged. The plugin
discards duplicate convar names and logs each one. The test suite also parses the
shipped file.

This file is replaced on every deployment. Copy live-server hotfixes back into
the repository or the next deploy will revert them.

Each convar rule supports:

| Field | Values |
| --- | --- |
| `tier` | `queried` or `userinfo`; a convar may use only one |
| `constraint` | `equals`, `max`, `range`, `minOrZero`, `off`, or `on` |
| `value`, `max` | Constraint bounds |
| `cheatProtected` | Wait until disabled `sv_cheats` has reached the client |
| `kickOnly` | Prevent escalation beyond a kick |

## Commands

| Command | Purpose |
| --- | --- |
| `anticheat_status` | Show gates, detector switches, dependency health, table sizes, and per-player evidence |
| `anticheat_reload` | Reload both configuration files and clear all evidence and punishment latches |
| `anticheat_dumpcmd <slot> [ticks=64]` | Log raw commands for one player |

`anticheat_status` is also published through the framework status service. A
zero table size under `detectionData` means the related detector is inactive
even if its switch is enabled.

### Cheat simulator

> Simulator commands rewrite decoded input used by the anticheat. The engine
> still receives the player's real command, but the detector sees synthetic
> data. Never enable this on a live player server.

Simulator commands are registered only when the plugin loads with
`debug.simulator` enabled, so changing the setting requires a plugin reload.
Each simulation lasts 10 seconds.

| Command | Simulation |
| --- | --- |
| `anticheat_sim_spin <target> [degPerSec=720]` | Spinbot |
| `anticheat_sim_jitter <target> [stepDeg=20]` | Repeating jitter |
| `anticheat_sim_badangles <target> [pitch=89.5]` | Impossible pitch and roll |
| `anticheat_sim_aimlock <target>` | Lock onto the nearest opponent |
| `anticheat_sim_mismatch <target> [deg=130]` | Fire angles that differ from visible aim |
| `anticheat_sim_nomouse <target>` | Lock onto the nearest opponent with the mouse counts zeroed |
| `anticheat_sim_off [target]` | Stop one simulation, or all if omitted |

Targets may be a slot or SteamID64. `anticheat_sim_aimlock` also exercises the
wallhack rules when the nearest opponent is behind cover. Aim assist needs
the slot calibrated first, so play normally for a few seconds before
`anticheat_sim_nomouse`. There is no silent-aim, triggerbot or recoil simulation
because those detectors evaluate real shots.

## Rollout

Deployment is currently held: `anticheat` is commented out in both the plugin
map and `box-a` entry in [`deploy/inventory.yml`](../../deploy/inventory.yml).

1. Deploy to `box-a` in `observe` mode with a webhook and collect at least one
   week of populated traffic.
2. Investigate suspicious evidence with `anticheat_dumpcmd`. Disable any
   detector that produces false positives.
3. Switch to `alert` for a second soak period.
4. Enable `ban` one detector at a time. Start with `invalidCvar`, then
   `namechanger` and `dllInjection`. Add `silentAim`, `antiAim`, `aimbot`,
   `recoil`, `triggerbot`, `aimAssist`, `wallhack` and `aimlock` last, with
   a soak between each change.

## Maintenance after CS2 updates

Revalidate these game-data entries after every game update. A stale entry may
crash, create false evidence, or silently disable detection.

| Entry | Failure mode |
| --- | --- |
| `CPlayer_MovementServices::RunCommand` | Crash on the first movement tick, unless the vtable slot check catches it |
| `CUserCmd::CSGOUserCmdPB` | Missing values silence aim modules; stale values can resemble valid angles |
| `CUserCmdBase::cmdNum` | Command chains collapse, silently disabling aimbot and part of antiaim |
| `CBaseEntity::Teleport` | Teleport grace stops suppressing discontinuities, so false positives appear |
| `CServerSideClient::ProcessRespondCvarValue` | The vtable slot stops holding code, so `Hooks.ClientConVars.Available()` fails |
| `CServerSideClientBase::m_nClientSlot` | A drifted offset still binds, so responses can reach the wrong player |
| `TraceShape` | The pattern stops matching, `Hooks.Trace.Available()` fails, and the wallhack module goes inert |
| `CCSPlayerPawn::m_iLastWeaponFireUsercmd`, `m_iShotsFired`, `m_bIsScoped`, `CCSPlayer_AimPunchServices::*` | Schema drift aborts the framework load; regenerate with `voltmod schemagen` |

The entries live in the framework's `gamedata/gamedata.jsonc`; its guide has the
re-verification procedure. `anticheat_status` exposes `teleportTracker` from
`Hooks.Teleport.Available()`, `sightLines` from `Hooks.Trace.Available()`, and
reports client convars as `degraded` when `Hooks.ClientConVars.Available()`
fails. In that state, network polling stops and `invalid_cvar` uses userinfo
only. Sight lines read `the engine has not traced yet this map` until the
first engine trace after a map start, which happens as soon as anything moves.

## Architecture

Detectors contain an SDK-free core over plain sample structures and a small
engine adapter. Core sources compile into `anticheat-tests` without linking the
game SDK.

```text
plugins/anticheat/
  src/
    Detectors.*    The cores plus the gates every adapter asks (enabled, eligible, report)
    Core/          Samples, geometry, lag estimate, weapon classes, and findings
    Correlation/   Shot correlator core, the engine feed, command sampling and sight probe
    Aim/           SDK-free aim rules: aimbot, aimlock, antiaim, silent aim, triggerbot,
                   recoil, mouse mismatch, wallhack
    Client/        Client integrity: DLL injection, invalid cvars, name changes
    Response/      Decision policy, actions, and Discord reporting
    Simulator/     Development-only input synthesis
  configs/         Per-server settings and shared detection data
  tests/           SDK-free detector and policy tests
```

`Detectors` owns every core by value and answers the four questions an adapter
asks (detections on, module on, slot eligible, report). Adapters hold a
`Detectors&`, so no header has to forward-declare the other side.

The shot correlator joins a command to the shot and events it produced. A
`weapon_fire` event binds to the command the pawn names as its last firing
command; without that number, only when exactly one candidate is in the window,
and it discards ambiguous candidates. Every shot is finalized once, two ticks
after it fired, and each shot-reading core judges it then. It retains 128 ticks of position history, and
leaves missing input-history indices absent rather than clamping them to another
command's angles.
