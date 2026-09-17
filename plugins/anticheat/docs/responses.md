# Responses

Every finding is logged and sent to the configured webhook. Mode controls any
additional action:

| Mode | Behavior |
| --- | --- |
| `observe` | Log and webhook only |
| `alert` | Also notify admins with ban access, rate-limited per finding |
| `ban` | Kick kick-only findings and ban other findings |

Mode is a ceiling rather than a separate axis: the confidence band decides how
far up the ladder a finding may go, and the mode caps it there. A `suspect`
finding never alerts, whatever the mode.

Safety rules:

- `m_yaw` outside its range, `fps_max` below 64, and refused
  cheat-protected convars remain kick-only.
- Punishment only escalates. Repeated findings cannot replace a ban with a kick
  or punish the same player twice.
- Whitelisted players and players without a resolved SteamID are logged and
  webhooked but not alerted or punished.
- Bans use the admin system's `Contracts::IAdminActions` interface. If the
  admin system is unavailable, bans degrade to logging; kicks still work.
- An empty `webhook.url` disables Discord. Reports are limited to one embed per
  player and detector per minute; failed sends are logged and not retried.

Punishment is recorded against the SteamID, so it survives a disconnect and a
map change; only `anticheat_reload` clears it.
