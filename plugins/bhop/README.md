# Bhop

Client-predicted auto-bunnyhop for CS2. Hold JUMP to hop, retain landing speed,
and build speed across chained hops.

## Why it feels smooth

Server-forced jumping can rubber-band because the client does not predict it.
This plugin keeps the client and server convars in agreement:

- In `enabled` mode, movement convars are set server-wide and replicated to every
  client, so all hops are client-predicted.
- In `grants` mode, a granted player's client receives the convar values through
  `CNETMsg_SetConVar`. The server applies the same values only around that
  player's `CPlayer_MovementServices::RunCommand` call, so other players are
  unaffected.

Speed comes from two layers:

1. **Air acceleration** (`sv_airaccelerate`, `sv_air_max_wishspeed`) lets players
   gain speed while strafing in the air. This is client-predicted.
2. **Hop-chain boost** (optional) scales horizontal velocity by `factor` for each
   jump within `chainWindowMs` of the previous jump, up to `maxSpeed`.

## Modes

| Mode | Behavior |
| --- | --- |
| `enabled` | Bhop is always on for everyone. Convar changes are restored on unload and re-applied at each round start. |
| `grants` | Bhop is off by default. `bhop_player` grants it for the current session; the grant is cleared on disconnect. |

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

The server applies one velocity correction per chained hop. If this feels rough
at high latency, lower `factor` or disable the boost. Convar-based movement
remains client-predicted.

## Admin-system integration

The admin-system plugin provides a **Bunnyhop** effect in its Effects menu
(permission flag `j`). It toggles a session grant through `bhop_player`. Run this
plugin in `grants` mode when admins should grant bhop to individual players. The
effect is inert if this plugin is not loaded.

## Maintenance

`grants` mode hooks `CPlayer_MovementServices::RunCommand` by vtable index
(`"RunCommand"` gamedata, currently Windows 22 / Linux 23). Re-check the index
against SwiftlyS2 or CS2Fixes gamedata after every CS2 update. `enabled` mode
uses no hook.
