# Anticheat

A server-side CS2 anticheat built around movement commands, shot correlation,
game events, and client-integrity checks. It has no client component or
detours, and its runtime analysis stays on the game thread.

The detectors adapt techniques from
[CS2AC](https://github.com/karola3vax/CS2AC) to the
[VoltMod framework](https://github.com/voltygg/voltmod).

> Start in `observe` mode. Validate real server traffic before enabling alerts
> or punishments.

## Data sources

The plugin consumes:

- Decoded `RunCommand` input, one user command per player per tick.
- `weapon_fire`, `bullet_impact`, `player_hurt`, `player_death`, and
  `player_spawn` events.
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
| `dll_injection` | Client event subscriptions unused by the stock HUD | First match |
| `invalid_cvar` | Client convars outside allowed values | First confirmed invalid value |
| `namechanger` | Repeated visible-name changes | 5 changes in 60 seconds |

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

Spin reports only after sustained angular speed with at least 0.85 directional
consistency. Jitter requires an exactly repeating yaw pattern, so a legitimate
180-degree bind does not report.

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

### Client integrity and names

`dll_injection` checks for unusual client event subscriptions 10 seconds after
full connection, then every 120 seconds.

`invalid_cvar` treats silence as no evidence. A refused cheat-protected value
counts only after three consecutive refusals and is always kick-only. Every
player/convar pair reports once and re-arms only after a valid result.

`namechanger` tracks distinct visible names. Its baseline stays current while
detection is gated off, so a name recorded during that gap cannot report later.

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

[`configs/settings.jsonc`](configs/settings.jsonc) is rendered per server from
the [deployment template](../../deploy/templates/plugins/anticheat/settings.jsonc).

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
| `anticheat_sim_off [target]` | Stop one simulation, or all if omitted |

Targets may be a slot or SteamID64. There is no silent-aim simulation because
that detector evaluates the real bullet impact.

## Rollout

Deployment is currently held: `anticheat` is commented out in both the plugin
map and `box-a` entry in [`deploy/inventory.yml`](../../deploy/inventory.yml).

1. Deploy to `box-a` in `observe` mode with a webhook and collect at least one
   week of populated traffic.
2. Investigate suspicious evidence with `anticheat_dumpcmd`. Disable any
   detector that produces false positives.
3. Switch to `alert` for a second soak period.
4. Enable `ban` one detector at a time. Start with `invalidCvar`, then
   `namechanger` and `dllInjection`. Add `silentAim`, `antiAim`, `aimbot`, and
   `aimlock` last, with a soak between each change.

## Maintenance after CS2 updates

Revalidate these game-data entries after every game update. A stale entry may
crash, create false evidence, or silently disable detection.

| Entry | Failure mode |
| --- | --- |
| `RunCommand` | Crash on the first movement tick, unless the vtable slot check catches it |
| `UserCmdPB` | Missing values silence aim modules; stale values can resemble valid angles |
| `UserCmdNumber` | Command chains collapse, silently disabling aimbot and part of antiaim |
| `Teleport` | Teleport grace stops suppressing discontinuities, so false positives appear |
| `ProcessRespondCvarValue` | Load-time bounds checks turn `Capability::ClientCvars` off |
| `ServerSideClientSlot` | Same, which is what stops responses reaching the wrong player |

The entries live in the framework's `gamedata/gamedata.jsonc`; its guide has the
re-verification procedure. `anticheat_status` reads `Runtime::Capabilities`: it exposes
`teleportTracker` from `Capability::Teleport` and reports client convars as `degraded` when
`Capability::ClientCvars` is off. In that state, network polling stops and `invalid_cvar` uses
userinfo only.

## Architecture

Detectors contain an SDK-free core over plain sample structures and a small
engine adapter. Core sources compile into `anticheat-tests` without linking the
game SDK.

```text
plugins/anticheat/
  src/
    Core/          Samples, geometry, weapon classes, and findings
    Correlation/   Shot correlator core and engine feed
    Detectors/     SDK-free rules and engine adapters
    Response/      Decision policy, actions, and Discord reporting
    Simulator/     Development-only input synthesis
  configs/         Per-server settings and shared detection data
  tests/           SDK-free detector and policy tests
```

The shot correlator joins a command to the shot and events it produced. A
`weapon_fire` event binds only when exactly one candidate is in the window; it
discards ambiguous candidates. It retains 128 ticks of position history, and
leaves missing input-history indices absent rather than clamping them to another
command's angles.
