# Anticheat

Server-side detection of blatant cheating - spinbot, aimlock/aim-snap, silent
aim/shot-angle divergence, no-flash, impossible view angles - from per-tick usercmd
analysis. Built on [CS2Kit](../../vendor/cs2-kit)'s `MovementHook` cmd feed
(`UserCmdView`: viewangles, buttons, mouse deltas, sub-tick moves, input history)
correlated with game events (`weapon_fire`, `player_hurt`, `player_death`,
`player_blind`).

## Design principle: correlation, not thresholds

Raw input anomalies alone never ban. Legit players fake-spin for fun and flick for
real - so every ban-tier signal requires the anomaly to be **correlated with
impossible accuracy, repeatedly**:

| Detector | Observe-tier signal | Ban-tier signal |
| --- | --- | --- |
| `spin` | Sustained yaw velocity ≥720 deg/s (subtick-unwrapped) | ≥3 kills (window 120s) whose fire-tick aim landed on the victim mid-spin; headshots score 1.5x |
| `aimSnap` | ≥40° pre-fire flick that settles before the shot | ≥4 snaps confirmed by on-target damage within 4 ticks of the fire (window 180s) |
| `shotAngle` | - | ≥3 shots whose fired angle (`input_history`) diverged ≥8° from the visible view and still connected within 4 ticks (window 180s) |
| `silentAim` | - | ≥3 large viewangle jumps with ~zero mouse input (`mousedx/dy`) confirmed by on-target damage. **Disabled by default**: the CS2 client's `mousedx/dy` population is unverified; `shotAngle` covers the same cheat class |
| `noFlash` | - | ≥3 kills while ≥1s of flash blindness remained (tracked via `player_blind`) |
| `sanity` | - | NaN/inf angles or pitch outside ±89.5° sustained 4 ticks (impossible via normal input) |

Scores decay over time (`decayPerSec`), so old evidence fades unless the pattern
repeats. "On-target" means the fire-tick view angle was within `onTargetEpsilonDeg`
(default 8°) of the victim's head or chest, computed from eye positions.

## Response ladder

`anticheat.mode` in `configs/settings.jsonc`:

| Mode | Behavior |
| --- | --- |
| `observe` (default) | Log every detection (`[AC] ...` lines, `Log::Warn`). Nothing else. |
| `alert` | Observe + notify online admins holding the Ban flag via admin-system (`as_ac_alert`), throttled by `alertCooldownSec`. |
| `ban` | Alert + auto-ban once a detector's score crosses `banScore` **and** its event count meets `minEvents` (`as_ac_ban`, latched once per player). |

Ship in `observe`, tune thresholds against real traffic (`anticheat_status`), soak in
`alert`, and only then flip to `ban`.

## Server console commands

| Command | Description |
| --- | --- |
| `anticheat_reload` | Re-read `settings.jsonc` without a restart (mode, thresholds, detector toggles). |
| `anticheat_status` | Dump every tracked player's live detector scores to the console. |
| `anticheat_dumpcmd <slot> [ticks=64]` | Log raw usercmds (angles, mouse, subticks, shot divergence) for one slot - the tool for verifying thresholds against real traffic. |

### Cheat simulator (test boxes only)

Guarded by `anticheat.debug.simulator.enabled`; it rewrites live player commands
through the kit's cmd filter, so leave it off outside an `-insecure` test server.
Each sim runs for 10s and targets a slot or steamid64:

| Command | Simulates |
| --- | --- |
| `anticheat_sim_spin <slot\|steamid64> [degPerSec=720]` | Spinbot (subtick-unwrapped yaw rotation). |
| `anticheat_sim_aimlock <slot\|steamid64> [snapDeg=45]` | Aim snap: one flick, then a still lock. |
| `anticheat_sim_silent <slot\|steamid64> [divergenceDeg=15]` | Shot-angle divergence: fired angle offset from the visible view. |
| `anticheat_sim_off [slot\|steamid64]` | Stop one sim, or all when omitted. |

## Admin-system integration

The anticheat is a standalone module; bans and alerts cross into admin-system over
its console bridge (the standard cross-plugin surface):

- `as_ac_ban <steamid64> <durationSec> <reason...>` - console-originated ban through
  `PunishmentManager` (persist + kick-if-online + broadcast) with
  `AdminName="AntiCheat"`, deliberately not counted against any admin's abuse stats.
- `as_ac_alert <steamid64> <detector> <score>` - translated chat line to every online
  admin with the Ban permission, suggesting a manual `!check` (cheat-check workflow).

Without admin-system loaded the commands are inert ("Unknown command") and the
anticheat degrades to observe-style logging.

## Configuration

`configs/settings.jsonc` (seeded on first deploy, never clobbered). Global keys:

| Key | Default | Meaning |
| --- | --- | --- |
| `anticheat.mode` | `"observe"` | Response ladder (see above). |
| `anticheat.alertCooldownSec` | `30` | Min seconds between admin alerts per player. |
| `anticheat.historyDepth` | `128` | Usercmd lookback samples per player (~2s at 64 tick). |
| `anticheat.ban.durationSec` | `0` | Auto-ban length; `0` = permanent. |
| `anticheat.ban.reason` | `"AntiCheat: ..."` | Ban reason; the detector tag is appended. |

Each detector block has `enabled`, its thresholds, `alertScore`/`banScore`,
`minEvents`/`eventWindowSec`, and `decayPerSec` - see the comments in
`settings.jsonc` for per-key meaning.

The `anticheat.debug` block holds test aids, all default-off:
`broadcastDetections` (chat-broadcast every detection, even in observe mode),
`dryRunBans` (broadcast "WOULD BAN ..." instead of banning), and
`simulator.enabled` (arm the cheat simulator commands above).

## Maintenance note

Two gamedata values drift with CS2 updates and must be re-verified against
SwiftlyS2/CS2Fixes after every game update:

- `"RunCommand"` - the hooked vtable index (wrong index = crash).
- `"UserCmdPB"` - byte offset of `CSGOUserCmdPB` inside the `CUserCmd` wrapper
  (missing = detectors see `Valid=false` and go quiet; stale = garbage reads).

Bots (`FL_FAKECLIENT`) and dead players are excluded from analysis. All work is
O(1) per tick per player plus event-driven lookbacks; there are no timers hotter
than the 1-per-tick usercmd feed the kit already decodes.
