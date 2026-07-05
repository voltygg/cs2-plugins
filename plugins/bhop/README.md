# Bhop

Smooth, ping-free auto-bunnyhop for CS2 servers: hold JUMP to hop, keep your landing
speed, and accelerate while chaining hops. Built on [CS2Kit](../../vendor/cs2-kit).

## Why it feels smooth

Server-forced jumping rubber-bands because the client doesn't predict it. This plugin
makes the **client itself** predict everything:

- In `enabled` mode, the movement convars are set server-wide through the engine, so
  they replicate to every client and all hops are client-predicted.
- In `grants` mode, a granted player's client receives the convar values via a
  per-client `CNETMsg_SetConVar` (its prediction auto-jumps), while the server flips
  the same convars only around that player's movement processing
  (`CPlayer_MovementServices::RunCommand` hook). Both sides agree; nobody else is
  affected.

Speed gain comes from two layers:

1. **Air acceleration** (`sv_airaccelerate`, `sv_air_max_wishspeed`) - strafe mid-air
   to gain speed, fully client-predicted.
2. **Hop-chain boost** (optional) - each jump chained within `chainWindowMs` of the
   previous one scales horizontal velocity by `factor`, capped at `maxSpeed`, so speed
   builds from simply holding JUMP.

## Modes

| Mode | Behavior |
| --- | --- |
| `enabled` | Bhop for everyone, always on. Convar changes are restored on unload and re-asserted each round start. |
| `grants` | Off by default. Individual players get session-only bhop via `bhop_player`; cleared on disconnect. |

## Server console commands

| Command | Description |
| --- | --- |
| `bhop_player <steamid64> <0\|1>` | Grant/revoke session bhop for a player (used by admin-system's Bunnyhop effect). |
| `bhop_reload` | Re-read `settings.jsonc` and re-apply the configuration without a restart. |

## Configuration

`configs/settings.jsonc` (seeded on first deploy, never clobbered):

| Key | Default | Meaning |
| --- | --- | --- |
| `bhop.mode` | `"enabled"` | `"enabled"` or `"grants"` (see above). |
| `bhop.autoBunnyhopping` | `true` | `sv_autobunnyhopping` - hold JUMP to hop. |
| `bhop.enableBunnyhopping` | `true` | `sv_enablebunnyhopping` - remove the landing speed clamp. |
| `bhop.staminaJumpCost` | `0.0` | `sv_staminajumpcost`; `-1` = leave untouched. |
| `bhop.staminaLandCost` | `0.0` | `sv_staminalandcost`; `-1` = leave untouched. |
| `bhop.airAccelerate` | `150.0` | `sv_airaccelerate` (game default 12); `-1` = untouched. |
| `bhop.airMaxWishSpeed` | `60.0` | `sv_air_max_wishspeed` (game default 30); `-1` = untouched. |
| `bhop.maxVelocity` | `-1` | `sv_maxvelocity`; `-1` = untouched. |
| `bhop.hopBoost.enabled` | `true` | Server-side chained-hop velocity boost. |
| `bhop.hopBoost.factor` | `1.08` | Horizontal velocity multiplier per chained hop. |
| `bhop.hopBoost.chainWindowMs` | `1000` | Jumps this close together count as chained. |
| `bhop.hopBoost.maxSpeed` | `1200.0` | Horizontal speed cap for the boost. |
| `bhop.notifyPlayer` | `true` | Center-message the player on grant/revoke. |

The hop boost is server-authoritative (one small velocity correction per hop, like a
boost pad). If it feels rough at very high ping, lower `factor` or disable it - the
convar layer alone is entirely client-predicted.

## Admin-system integration

The admin-system plugin ships a **Bunnyhop** effect (admin menu → Effects, permission
flag `j`) that toggles a session grant through `bhop_player`. Run this plugin in
`grants` mode on regular servers so admins can hand out bhop for fun; without the bhop
plugin loaded the toggle is inert.

## Maintenance note

`grants` mode hooks `CPlayer_MovementServices::RunCommand` by vtable index (gamedata
`"RunCommand"`, currently windows 22 / linux 23). The index drifts with CS2 updates -
re-verify it against SwiftlyS2/CS2Fixes gamedata after every game update. `enabled`
mode uses no hooks and is update-proof.
