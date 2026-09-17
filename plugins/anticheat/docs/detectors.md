# Detectors

What each detector measures, and what this plugin deliberately cannot see. The
switches and thresholds are tabled in the [README](../README.md); how the
numbers add up is in [Suspicion](suspicion.md).

## Aimbot

Aimbot only evaluates commands that damaged an opponent. It scans up to 32
strictly adjacent commands and looks for rapid convergence: more than 10
degrees landing within 20% of the previous aim error, or more than 5 degrees
landing within 10%. It also detects a one-command snap and return. The shot
must be at least 100 units away, and both players must be outside the
five-second teleport grace period.

## Aimlock

Aimlock needs 1.5 seconds with at least 95% of samples inside one target's
angular width while that target travels at least 48 angular units. A stationary
target or two targets under the crosshair produces no episode.

To account for interpolation, it evaluates lag hypotheses derived from
round-trip time and `cl_interp_ratio`, including plus or minus two ticks. If
network timing is unusable, it records no evidence. Death ends the current
episode but keeps completed episodes.

## Antiaim

Antiaim weighs each rule out of a hundred:

- Pitch beyond 89.01 degrees or roll beyond 50.01 degrees adds 2.
- A structurally impossible command adds 1.
- A base view at least 120 degrees from the command's fire direction adds 0.4,
  at most once every four commands. It is worth less than the others because it
  used to be discounted by decaying two and a half times faster instead.
- A one-command attack return adds 5.

Its evidence arrives at command rate, so it fades on a thirty-second half-life
rather than the ten minutes the shot-driven rules use.

Spin and jitter walk the client's own command sequence, so server-side batching
or a lagging connection does not break an episode. Spin reports after sustained
one-direction rotation with at least 0.85 directional consistency: 10 seconds
above 320 degrees a second, 6 above 1000, or 3 above 2200. Jitter requires an
exactly repeating yaw pattern for 5 seconds, so a legitimate 180-degree bind does
not report.

## Silent aim

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

## Triggerbot

Triggerbot measures reaction time the way a server can: each tick, for every
enemy and every lag hypothesis, it records when the crosshair came to rest on
that enemy's hull in the world the client was looking at. When a shot hurts
that enemy and it is the first shot of a burst (no fire in the previous 10
ticks), the reaction is the fire tick minus the *earliest* of those rests, so a
wrong lag guess can only make the reaction look slower. The crosshair must have
moved less than 2 degrees around the crossing: the enemy walked in, the shooter
did not flick. A reaction of 3 ticks or less scores 2, 4 to 6 ticks scores 1,
and 8 points are one whole unit of suspicion. Human reactions land at 10 ticks
and more.

## Recoil control

Recoil control compares the view with the recoil punch across a spray. The
punch the client predicted for each command is read from the pawn as the
command arrives, so the punch a shot left behind is the one on the command
after it. Over a spray of at least 8 shots on one weapon with no gap above 16
ticks, the view change between consecutive shots is fitted against the punch
change with one factor, once for the same command and once for the command
after. A fit with a factor between 0.5 and 2.5, at least 3 degrees of punch,
and a residual of 0.35 degrees RMS or less per shot marks the spray; three
marked sprays are one whole unit of suspicion. The pattern is public, but the
residual
requires cancelling the punch the engine actually produced, tick by tick.

## Aim assist

Aim assist learns each player's degrees per mouse count from their own
turns: the median of the last 32 ratios between the yaw change and the counts
the command carried, accepted once 16 samples agree within 25%. A client whose
counts never agree with its turns (a controller, or a client that does not fill
them) is never calibrated and never judged. Once calibrated, a command whose
turn differs from the counts by more than four counts' worth and more than one
degree is unexplained. It counts only when it moved the aim onto an enemy: from
outside to within 3 degrees, closing at least half of the unexplained turn.
Keyboard turning and scoped commands are skipped. Six such turns are one whole
unit of suspicion. An aimbot that moves the real mouse instead of the view escapes
this rule and is left to the others.

## Wallhack

Wallhack evidence comes from sight lines. Every tick the plugin traces from
each checked player's eyes to the head, chest and feet of the enemy nearest
their crosshair, ignoring both pawns, against world geometry and line-of-sight
blockers only (windows and clips do not hide anyone). Three rules score into
the same score, and 6 points are one whole unit of suspicion:

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

Without a usable sight line (`sightLines` in `anticheat_status`) this rule is
inert: no trace, no evidence.

## Client integrity and names

`dll_injection` checks for unusual client event subscriptions 10 seconds after
full connection, then every 120 seconds.

`invalid_cvar` treats silence as no evidence. A refused cheat-protected value
counts only after three consecutive refusals and is always kick-only. Every
player/convar pair reports once and re-arms only after a valid result.

`namechanger` tracks the scoreboard name and the clan tag together. It reads the
controller eight times a second as well as on settings changes, because a tag pushed
by a cheat does not always raise one. Its baseline stays current while detection
is gated off, so a name recorded during that gap cannot report later. After a
burst of five changes the slot goes quiet for a minute, so an animated tag is
one offence rather than one a second.

## What it cannot see

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
