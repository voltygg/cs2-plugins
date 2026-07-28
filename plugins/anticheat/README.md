# Anticheat

Server-side cheat detection for CS2 community servers: aim analysis over correlated
shots plus client-integrity checks. The detection algorithms are reimplementations of
the ones in [CS2AC](https://github.com/karola3vax/CS2AC) (karola3vax, AGPL-3.0) - the
approach and the tuning are theirs, the code here is written against
[CS2Kit](../../vendor/cs2-kit) and its sample structs.

Everything runs off two feeds the kit already provides: the `RunCommand` movement hook
(one decoded `UserCmdView` per player per tick) and game events (`weapon_fire`,
`bullet_impact`, `player_hurt`, `player_death`, `player_spawn`, settings changes). There
are no detours, no client-side component, and nothing on a thread other than the game
thread.

## Architecture

Every detection is a **core**: an SDK-free class over plain sample structs
(`Core/Samples.hpp`), with all of its tuned constants `constexpr` at the top of its own
source file and no config surface at all. Cores are unit-tested directly - the whole
detection layer compiles into `anticheat-tests` without linking the SDK (122 doctest
cases). The SDK lives in thin adapters that convert engine state into samples and hand
verdicts to the response funnel. The backbone is the **ShotCorrelator**: it joins a
usercmd to the shot it fired and to the events that shot produced, so every aim module
reasons about "this exact command fired this exact bullet at this exact world state".
Matching is deliberately strict - a `weapon_fire` is only bound to a command when
*exactly one* candidate sits in the window, and an ambiguous window burns all its
candidates so a later event cannot claim one arbitrarily. Alongside it the correlator
keeps 128 ticks of per-slot position frames (origin, eye, team, alive, teleported), and
the firing angle always comes from the command's input history at
`Attack1StartHistoryIndex` - an index the transport cap dropped means *absent*, never
clamped back into range, because clamping reads another shot's angles.

```text
plugins/anticheat/
  src/
    Core/          Samples, Geometry (great-circle angle math), WeaponClass, Finding
    Correlation/   ShotCorrelatorCore (SDK-free) + ShotCorrelator (the single engine feed)
    Detectors/     *Core.cpp = SDK-free rules; *Detector.cpp = engine adapter
    Response/      FunnelPolicy (pure decision logic), ResponseManager, DiscordReporter
    Simulator/     CheatSimulator (dev-only input synthesis)
  configs/settings.jsonc
  tests/
```

Global gates, all in `AntiCheatManager`: the master `enabled` switch; `sv_cheats` on
disables every detection unless `allowSvCheatsTesting`; only connected, non-bot humans
are judged; a map change, an `anticheat_reload`, an `sv_cheats` flip or an
`mp_teammates_are_enemies` change drops every piece of accumulated evidence.

## Detections

Seven kill switches under `anticheat.detections`. Thresholds are compiled in.

### `aimbot` - snap-onto-target

Judges only commands that actually damaged an enemy. From the damaging command it walks
backwards along *strictly* adjacent commands (command number -1, client tick exactly -1,
server tick gap 0 or 1; one hole ends the chain) for up to 32 ticks and looks for
convergence: a jump larger than 10 deg that leaves the aim inside 20% of its previous
error, or larger than 5 deg inside 10%. A second rule catches the one-command
snap-return - the shot's angle more than 5x off a line its neighbours share within
10 deg. Four incidents in 10 minutes report. Guards: minimum 100 units to the victim,
both parties outside the 5 s teleport/spawn grace, opponents only
(`mp_teammates_are_enemies` aware), and any non-finite value ends the chain.

### `aimlock` - inhuman tracking

An episode is 96 ticks (1.5 s) of aim staying inside the target's angular width for at
least 95% of samples, while the target moves at least 48 units of angular travel across
the episode - a still target proves nothing. The target must be unambiguous: if two
players are simultaneously under the crosshair, no episode starts. Because a client aims
at an interpolated past, the module carries a *set* of lag hypotheses (round trip plus
`cl_interp_ratio`, searched +/-2 ticks) and requires one of them to survive the whole
episode. Network sanity is a hard gate: no live channel, RTT outside 0-2 s, or a
non-numeric interp ratio means no estimate and therefore never evidence. Three episodes
in 10 minutes report, after which the module stays quiet until the player has been off
that target for half a second. Minimum range 200 units; teleported or dead players are
skipped - dying drops the in-progress episode but not the episodes already counted, so a
cheat cannot clear its own record by dying between them.

### `antiaim` - impossible and fabricated angles

One decaying score (threshold 100, -2/s, mismatch evidence -5/s). Pitch beyond 89.01 deg
or roll beyond 50.01 deg adds 2. A structurally impossible command - non-finite base,
history or subtick angles, or an attack index outside `[-1, historyTotalCount)` - adds 1.
A base view 120 deg or more from the angles the same command claims it fired along adds 1,
at most every 4 commands, since a fast legitimate flick can do it once. A one-command
attack return (shot angle more than 30 deg and 5x off neighbours within 10 deg of each
other) adds 5. The 5 s spawn/teleport grace wipes episode state, because origin and
angles jump discontinuously there, and continuous conditions are suppressed so a stuck
bad angle cannot re-fire every tick.

The motion half (spin and jitter) scores out instantly instead. Both run over a
newest-first run of up to 20 commands on strictly consecutive server ticks - a lost tick
would make any rate computed across it a fiction. Spin needs 16 samples whose mean and
latest yaw rate clear a tier and whose direction consistency is at least 0.85 (this is
what separates a spinbot from a player flicking back and forth), sustained for 15 s at
320 deg/s, 10 s at 1000, or 10 s at 2200, with one second of interruption forgiven.
Jitter needs the yaw sequence to repeat on a period of 2, 3 or 5 commands, four full
repetitions within 0.25 deg, spanning more than 10 deg, sustained 10 s - so a legitimate
180 bind, which looks identical for a moment, never qualifies.

### `silentaim` - fire angle vs. bullet

Compares the pawn's visible eye angles at `weapon_fire` with where the bullet actually
landed. Only shots that both hurt someone *and* reported an impact are scored, two ticks
after the fire, once every event they can produce has arrived. The deviation ceiling is
per weapon class - 2.5 deg for snipers, 4.3 for pistols, 12.5 for rifles, 22.5 for SMGs
(spray plus movement inaccuracy legitimately throws SMG bullets far off the crosshair).
A qualifying shot scores 2 (1 if airborne, where inaccuracy makes a wide shot cheap
evidence) or 3 when the deviation is past every weapon's ceiling, plus 1 for a headshot
and 1 for a wallbang; 12 points in 10 minutes report. Impacts closer than 100 or farther
than 10000 units are ignored.

### `dll_injection` - blacklisted event subscriptions

A stock client subscribes only to the events its HUD needs. Injected client code
registers its own legacy listener and asks for events no HUD ever wants; the server can
read that without touching the client. 117 such names are checked
(`Detectors/DllEventBlacklist.hpp`), and any hit reports. The first scan waits 10 s after
full connect because a client's listener does not exist the instant it joins - one grace
retry if it is still absent, then a 120 s rescan cadence.

### `invalid_cvar` - client settings

Nine client convars are polled over `CSVCMsg_GetCvarValue`, four per poll in rotation
(the kit refuses a slot's twelfth outstanding query) at a per-slot interval randomized
between 1 and 5 s so the schedule is not predictable. `sensitivity` and `m_yaw` are read
straight from userinfo instead, where they are always available - and *only* there, since
two tiers judging one convar would share a single latch and keep flipping it against each
other. Rules: `m_yaw` at most 0.3, `fps_max` 0 or at least 64, `sensitivity` between
0.0001 and 20, `cl_pitchdown` and `cl_pitchup` exactly 89, `cl_yawspeed` exactly 210,
and - only while `sv_cheats` is off and 30 s past its last disable -
`sv_cheats`/`cl_showpos`/`cam_showangles` off, `cl_drawhud` on, `fov_cs_debug` 0. Silence
is never evidence: a client is under no obligation to answer. A reply that *refuses* the
value counts only for the cheat-protected names, only after three such replies in a row
for the same convar (any reply carrying a value restarts the count), and only as a kick -
so a convar a future engine update removes cannot kick a whole server off one poll. Each
(slot, cvar) latches - it reports once and re-arms only after reading valid in between.

### `namechanger` - name spam

Five distinct visible name changes within 60 s. The baseline is taken at full connect and
kept up to date even while detections are gated off, so a change is never measured
against a stale name.

## Response funnel

Every detection is logged (`[AC] ...`, `Log::Warn`) and sent to the webhook regardless of
mode. What happens beyond that is `anticheat.mode`:

| Mode | Behavior |
| --- | --- |
| `observe` (default) | Log and report only. This is the dry run. |
| `alert` | Also notifies admins holding the Ban flag via `as_ac_alert`, one per (player, detection) per 30 s. |
| `ban` | Also punishes: `KickOnly` findings kick, everything else bans via `as_ac_ban`. |

- **Kick-only set.** Findings whose false-positive cost must stay recoverable never ban,
  even in `ban` mode: `m_yaw` out of range, `fps_max` below 64, and a cheat-protected
  convar the client refused to report. Kicks are issued in-process; bans go out over the
  console bridge.
- **No-downgrade latch.** A slot's punishment level only ever rises, so a later detection
  cannot demote an issued ban to a kick and a repeat cannot double-punish. Cleared on
  disconnect, map change and reload.
- **Whitelist.** SteamID64s in `whitelistSteamIds` are logged and webhooked but never
  alerted and never punished. A player whose SteamID is not resolved yet is treated the
  same way.
- **Admin-system bridge.** `as_ac_ban <steamid64> <durationSec> <reason...>` and
  `as_ac_alert <steamid64> <detection> <score>` are admin-system's console commands, so
  persistence, kick-if-online and broadcast stay in one place. Reasons are stripped of
  console-structural characters and capped at 200 chars. Without admin-system loaded both
  are "Unknown command" and the anticheat degrades to logging (kicks still work).
- **Discord.** Optional, dormant while `webhook.url` is empty. One embed per (player,
  detection) per minute; delivery failures log once and are never retried.

## Configuration

`configs/settings.jsonc` - the whole file, one `anticheat` section, no translations. The
deployed copy is rendered from
[`deploy/templates/plugins/anticheat/settings.jsonc`](../../deploy/templates/plugins/anticheat/settings.jsonc),
which carries the same keys (box-a arms the simulator).

| Key | Default | Meaning |
| --- | --- | --- |
| `enabled` | `true` | Master switch; off means no hook feeds the cores. |
| `mode` | `"observe"` | Response funnel: `observe`, `alert`, `ban`. |
| `banDurationSec` | `0` | Auto-ban length in seconds; `0` = permanent. |
| `whitelistSteamIds` | `[]` | SteamID64s reported but never punished. |
| `allowSvCheatsTesting` | `false` | Keep detecting while `sv_cheats` is on. Test boxes only. |
| `detections.aimbot` | `true` | Snap-onto-target. |
| `detections.aimlock` | `true` | Inhuman tracking episodes. |
| `detections.antiAim` | `true` | Impossible/fabricated angles, spin, jitter. |
| `detections.silentAim` | `true` | Fire angle vs. bullet impact. |
| `detections.dllInjection` | `true` | Blacklisted client event subscriptions. |
| `detections.invalidCvar` | `true` | Client convar rules. |
| `detections.namechanger` | `true` | Name-change spam. |
| `webhook.url` | `""` | Discord webhook; empty disables reporting. |
| `debug.simulator` | `false` | Arm the cheat simulator commands (dev boxes only). |

## Server console commands

| Command | Description |
| --- | --- |
| `anticheat_status` | Global JSON snapshot (mode, gates, module toggles, `clientCvars` availability, teleport tracker, correlator frames) plus one line per human player: punishment level, per-module counters, latched convars, pending queries, next poll, shots and commands held. The same snapshot is registered as the `anticheat` section of the kit's `Engine().Status`. |
| `anticheat_reload` | Re-read `settings.jsonc` **and drop all accumulated evidence**, including the punishment latch. |
| `anticheat_dumpcmd <slot> [ticks=64]` | Log raw usercmds for one slot: view, mouse deltas, buttons, subtick deltas, input-history counts, and what the attack index resolved to (present / capped away / out of range). The tool for checking a suspicion against real traffic. |

### Cheat simulator

Gated by `anticheat.debug.simulator`. The commands are only registered at load, so
enabling the flag needs a plugin reload, not just `anticheat_reload`.

**These rewrite live decoded input.** The filter edits the `UserCmdView` every downstream
listener sees; the engine still runs the player's real command, so gameplay is
unaffected, but that player's readings become fiction for the duration. Never arm this on
a server people are playing on.

| Command | Simulates |
| --- | --- |
| `anticheat_sim_spin <slot\|steamid64> [degPerSec=720]` | Spinbot (yaw rotation with matching subtick delta). |
| `anticheat_sim_jitter <slot\|steamid64> [stepDeg=20]` | Exactly repeating three-yaw jitter bind. |
| `anticheat_sim_badangles <slot\|steamid64> [pitch=89.5]` | Impossible pitch and roll. |
| `anticheat_sim_aimlock <slot\|steamid64>` | Locks onto the nearest opponent's chest. |
| `anticheat_sim_mismatch <slot\|steamid64> [deg=130]` | Input-history angles diverging from the visible view, which drives AntiAim's fired-vs-visible mismatch rule. |
| `anticheat_sim_off [slot\|steamid64]` | Stop one sim, or all when omitted. |

Each sim runs for 10 s. SilentAim has no simulator: it is scored from where the bullet actually
landed, and the filter only edits the decoded view - the usercmd the engine simulates is untouched.

## Rollout

Deployment is currently held: `anticheat` is commented out in
[`deploy/inventory.yml`](../../deploy/inventory.yml) for both the plugin list and box-a's
server entry. Uncomment both to ship it.

1. **Observe.** Deploy to box-a in `observe` with `webhook.url` set. Nothing is ever done
   to a player; every detection lands in the log and in Discord. Soak through real
   traffic - a week of populated hours, not an empty server.
2. **Read the misses.** For anything that looks wrong, `anticheat_dumpcmd` the slot and
   compare against the evidence string. A module producing false positives gets its
   toggle turned off; thresholds are not operator-tunable by design.
3. **Alert.** Flip to `alert` once the observe stream is clean. Admins now get pinged and
   can `!cc` a suspect manually - a second soak, with humans as the funnel.
4. **Ban, module by module.** Flip to `ban` with only the modules you trust enabled.
   Start with the kick-only rules (`invalidCvar` alone - the worst case is a recoverable
   kick), then `namechanger` and `dllInjection`, and enable the aim modules
   (`silentAim`, then `antiAim`, then `aimbot` and `aimlock`) last, one at a time, with a
   soak between each.

## Maintenance

Four gamedata values drift with CS2 updates and must be re-verified against
SwiftlyS2/CS2Fixes/CS2AC after every game update:

| Entry | Used by | If it drifts |
| --- | --- | --- |
| `RunCommand` | `MovementHook` vtable index | **Crash** on the first movement tick. |
| `UserCmdPB` | usercmd decode | Missing: `Valid=false` views and the aim modules go silent. Stale: garbage angles and buttons, which looks like plausible data. |
| `ProcessRespondCvarValue` | `ClientCvarService` vtable index | Sanity-bounded at init, so the stage degrades instead of hooking an unrelated vfunc. |
| `ServerSideClientSlot` | `ClientCvarService` slot offset | Sanity-bounded too; unchecked it would attribute one client's answer to another player. |

The two `ClientCvars` offsets degrade rather than crash: `anticheat_status` reports
`"clientCvars": "degraded"`, the network convar poll stops, and `invalid_cvar` falls back
to the two userinfo values. Check that field after every game update - a quiet detection
looks exactly like a clean server.
