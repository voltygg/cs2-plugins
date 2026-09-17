# Configuration

## Operator settings

[`configs/settings.jsonc`](../configs/settings.jsonc) ships with the plugin. A
deployment overrides individual keys under `plugins.anticheat.settings` in
[`deploy/inventory.yml`](../../../deploy/inventory.yml).

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

## Detection data

[`configs/detections.jsonc`](../configs/detections.jsonc) contains blacklisted
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
