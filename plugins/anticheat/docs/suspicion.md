# Suspicion

No detector decides on its own whether somebody is cheating. Each one weighs
what it saw and adds it to that player's suspicion, and the score decides what
is worth reporting.

## One shared scale

Suspicion is measured in whole units. **One unit is one detector confident on
its own**, so a detector that reports after four incidents adds a quarter each
time, and one that reports after twelve weighted points adds a twelfth per
point.

That division lives in the rule, next to the constant it divides by:

```cpp
/** Four snap-hit incidents are what this rule reports on alone. */
static constexpr float PerIncident = 1.0f / 4.0f;
```

Keeping it there is what lets the score stay free of a per-detector table, and
what makes the number readable where it is earned rather than in a list
somewhere else.

The point of one scale is that partial evidence adds up. Three quarters of
`aimbot` plus a third of `wallhack` is more than one unit, and neither detector
would ever have spoken alone. Under the old design each detector counted by
itself, so a player two thirds of the way along four different detectors
produced nothing at all.

## Fading, not expiring

Evidence fades on a half-life. A window with a hard edge meant an incident
counted fully at nine minutes and not at all at eleven; a half-life means it
simply matters less as it ages.

| Half-life | Used by | Why |
| --- | --- | --- |
| 10 minutes | The shot-driven rules | Matches how often a shot produces evidence |
| 30 seconds | `antiaim` | Its evidence arrives at command rate, so the fade is a rate limit rather than a memory |
| 1 hour | `dll_injection`, `invalid_cvar` | A confirmed fact about the client stays true while they are connected |

## Reporting never clears the score

A detection used to zero the counter that produced it, which handed a cheat a
clean ten minutes before the same detector could speak again. Now the evidence
stays and the response escalates instead.

Each band reports once. It speaks again only after the score decays back below
three quarters of that band, so a player sitting at a threshold does not become
a stream of alerts. The band a player has reached is remembered across a
reconnect, so returning does not re-report what it already did.

`certain` additionally requires one detector to be confident on its own.
Evidence fused from several partial detectors can raise an alert but can never
get somebody punished by itself, which is what keeps several noisy detectors
from summing into a ban.

## Two kinds of state, two lifetimes

- **In-flight detector state** - tracking episodes, command history, mouse
  calibration - is keyed by ticks and positions. A map change, a `sv_cheats`
  change or a `mp_teammates_are_enemies` change drops it, because the ticks and
  positions it refers to no longer mean anything.
- **Suspicion** is the player's, and follows them. It survives a map change, and
  a reconnect hands it back, keyed by SteamID. Only `anticheat_reload` clears
  it.

Carried evidence is forgotten once it decays below a twentieth of a unit, so
the table holds players still under suspicion rather than everyone the server
has ever seen.

## What it does not measure

Exposure. Four incidents mean the same whether the player fired fifteen shots
or six hundred, so a high-volume player accumulates evidence faster than a
cautious one for the same rate of cheating.

Normalizing by shots fired belongs in the points each rule contributes, and
needs a rolling denominator from the shot history. It is not implemented, and
the current design makes it a change inside one rule rather than a change to
the score.
