# Bhop

Bhop provides server-authoritative bunny-hop assistance while preserving
client-side movement prediction. It can enable autobhop for everyone or grant
it per player through the admin system.

## Features

- Client-predicted autobhop without repeated correction from the server.
- Optional stamina removal and movement-convar tuning.
- Configurable velocity boost with a speed cap.
- Global and per-player operating modes.
- Runtime configuration reloads.

## Operating modes

| Mode | Behavior |
| --- | --- |
| `enabled` | Applies bhop behavior to every eligible player |
| `grants` | Applies behavior only to players granted bhop access |
| `disabled` | Leaves player movement unchanged |

In `grants` mode, the plugin integrates with the admin system. The `j` admin
flag controls access, and the admin menu can toggle Bhop for a player.

## Prediction behavior

The plugin updates replicated movement convars so clients predict the same
movement rules as the server. In grants mode it temporarily applies movement
settings around each granted player's command processing, so the effect stays
scoped to that player.

## Server commands

| Command | Purpose |
| --- | --- |
| `bhop_player <steamid64> <0\|1>` | Disable or enable bhop for one player |
| `bhop_reload` | Reload the plugin configuration |

`bhop_player` is intended for server-side use and automation.

## Configuration

Edit `game/csgo/addons/bhop/configs/settings.jsonc` in the server installation.

| Setting | Default | Purpose |
| --- | --- | --- |
| `mode` | `enabled` | Select `enabled`, `grants`, or `disabled` |
| `autobhop` | `true` | Hold jump to jump again on landing |
| `enableBhop` | `true` | Enable bunny-hop movement behavior |
| `stamina` | `0` | Configure landing stamina |
| `airAccelerate` | `150` | Set airborne acceleration |
| `airMaxWishSpeed` | `60` | Set maximum airborne wish speed |
| `maxVelocity` | `-1` | Set the engine velocity limit; `-1` leaves it unchanged |
| `boost.enabled` | `true` | Enable landing-window velocity boosts |
| `boost.factor` | `1.08` | Multiply horizontal velocity by this value |
| `boost.windowMs` | `1000` | Limit boosts to this landing window |
| `boost.maxSpeed` | `1200` | Cap boosted horizontal speed |
| `notify` | `true` | Notify players when their bhop state changes |

Reload the configuration with `bhop_reload` after editing it.

## Maintenance

The movement hook depends on platform-specific game data. The current command
indices are Windows `22` and Linux `23`. Revalidate them after CS2 updates if
the hook stops running or players receive prediction errors.

For shared build and development commands, see the
[repository README](../../README.md).
