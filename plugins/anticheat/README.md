# Anticheat

A server-side CS2 anticheat built around movement commands, shot correlation,
sight lines, game events, and client-integrity checks. It has no client
component, and its runtime analysis stays on the game thread.

The detectors adapt techniques from
[CS2AC](https://github.com/karola3vax/CS2AC) to the
[VoltMod framework](https://github.com/voltygg/voltmod).

> Start in `observe` mode. Validate real server traffic before enabling alerts
> or punishments.

## How it decides

No detector decides on its own whether somebody is cheating. Each one weighs
what it saw and adds it to that player's **suspicion**, measured in whole units
where one unit is one detector confident on its own. Partial evidence adds up,
so three quarters of `aimbot` plus a third of `wallhack` reports even though
neither detector would ever have spoken alone.

| Suspicion | Confidence | What happens |
| --- | --- | --- |
| 1.0 | `suspect` | Logged and sent to the webhook |
| 2.0 | `likely` | Admins alerted as well |
| 3.0 | `certain` | Kick or ban, in `ban` mode |

Evidence fades on a half-life rather than expiring on a window edge, and
reporting never clears it. [Suspicion](docs/suspicion.md) has the full model.

## Detectors

Each detector has a kill switch under `anticheat.detections`. What each one
contributes is compiled in, beside the rule that earns it.

| Detector | Looks for | Reaches one unit at |
| --- | --- | --- |
| `aimbot` | Aim snapping onto the player damaged by that command | 4 snap-hit incidents |
| `aimlock` | Aim held inside a moving target's angular width | 3 tracking episodes |
| `antiaim` | Impossible angles, fabricated fire angles, spin, and jitter | 100 weight; spin and jitter reach it alone |
| `silentaim` | Impacts far from the visible aim direction | 12 weighted points |
| `triggerbot` | Hits within 47-94 ms of an enemy walking into a resting crosshair | 8 weighted points |
| `recoil` | Sprays whose view cancels the actual recoil punch | 3 marked sprays |
| `aim_assist` | View turns the mouse counts cannot explain that land on an enemy | 6 unexplained turns |
| `wallhack` | Following, pre-aiming and shooting enemies through cover | 6 weighted points |
| `dll_injection` | Client event subscriptions unused by the stock HUD | First match |
| `invalid_cvar` | Client convars outside allowed values | First confirmed invalid value |
| `namechanger` | Repeated name or clan tag changes | 5 changes in 60 seconds |

All detectors require the master switch, a connected human player, and
`sv_cheats` off unless `allowSvCheatsTesting` is enabled.
[Detectors](docs/detectors.md) explains what each one measures, and what this
plugin deliberately cannot see.

## Documentation

| Guide | What is in it |
| --- | --- |
| [Suspicion](docs/suspicion.md) | The shared score, the bands, and what state lives how long |
| [Detectors](docs/detectors.md) | What each detector measures, and the blind spots |
| [Responses](docs/responses.md) | Modes, alerts, punishment, and the safety rules |
| [Configuration](docs/configuration.md) | Operator settings and the shared detection data |
| [Commands](docs/commands.md) | Console commands, the cheat simulator, and checking a headless server |
| [Operations](docs/operations.md) | Rollout order and what to revalidate after a CS2 update |
| [Architecture](docs/architecture.md) | Source layout, the pure/engine split, and shot correlation |
