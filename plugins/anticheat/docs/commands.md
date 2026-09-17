# Commands

| Command | Purpose |
| --- | --- |
| `anticheat_status` | Show gates, detector switches, dependency health, table sizes, and per-player evidence |
| `anticheat_reload` | Reload both configuration files and clear all evidence and punishment latches |
| `anticheat_dumpcmd <slot> [ticks=64]` | Log raw commands for one player |

`anticheat_status` is also published through the framework status service. A
zero table size under `detectionData` means the related detector is inactive
even if its switch is enabled.

## Cheat simulator

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

## Checking a headless server

`anticheat_status` prints the gates and one line per checked player over
the console or RCON. With `debug.includeBots` and `debug.simulator` on, bots are
checked like humans and `anticheat_sim_spin`, `anticheat_sim_jitter`,
`anticheat_sim_badangles`, `anticheat_sim_names`, `anticheat_sim_aimlock`,
`anticheat_sim_nomouse` and `anticheat_sim_mismatch` drive a slot through each
detector for ten seconds; a
finding then appears in the log as `[AC] <DETECTOR> on <name>`. Bots run no
input history, so their `cmds=` counter stays at zero. Keep both switches off in
production.
