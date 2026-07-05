# Admin System

A full admin suite for CS2 community servers: punishments, fun effects, a WASD admin menu, multi-server admin groups sharing one database, abuse protection with an audit trail, and cheat-check workflows.

## Features

- **Punish:** Kick, ban, mute, gag, warn (warnings auto-escalate to a ban at a configurable threshold)
- **Control:** Slay, Bring, Goto, Freeze, Noclip, Health/Armor presets, Godmode (FL_GODMODE), Bury/Unbury, Change Team
- **Effects:** Ghost (translucent render), Disco (color cycling), Launch (high-velocity yeet + 3 s fall protect), Smite (theatrical instakill), Swap (exchange two players' positions), Bunnyhop (session bhop grant via the [bhop plugin](../bhop/README.md) in `grants` mode - survives death, cleared on disconnect; inert without that plugin). Blind reserved (awaits Fade user-message infra).
- **Admin System:** Permission flags (`a` freeze-admins, `b` hide/who, `c` kick, `d` ban, `e` unban, `o` voice-mute, `p` text-mute, `q` warn, `s` control, `h` health/cheats, `f` fun, `j` bhop, `k` cheat-check, `r` admin menu, `z` root), groups, immunity levels. Self-targeting always allowed.
- **Multi-Server:** Several servers share one database; admins can hold different groups per server (see [Multi-Server Setup](#multi-server-setup))
- **Abuse Protection:** Automatic + manual freezing of rogue admins, with a full action audit trail (see [Admin Abuse Protection](#admin-abuse-protection))
- **WASD Menus:** Top-level category dispatcher → player picker → actions. Toggle entries (Ghost, Disco, Godmode) show live `: ON / : OFF` state.
- **Database:** PostgreSQL, async-first (a worker thread owns the connection; gameplay reads hit in-memory caches, writes ride the worker) with forward-only auto-applied migrations
- **Chat Commands:** `!kick`, `!ban`, `!voice_mute`, `!text_mute`, `!warn` and more

## Requirements

- CS2 Dedicated Server
- [Metamod:Source 2.0](https://www.sourcemm.net/)
- PostgreSQL 18+

## Installation

1. Download the latest release from [Releases](https://github.com/m9snoi-net/cs2-plugins/releases)
2. Extract to your server's `csgo/` folder
3. Configure database and plugin settings in `addons/admin-system/configs/settings.jsonc` (set a unique `server.tag` per server - see [Multi-Server Setup](#multi-server-setup))
4. (Optional) The plugin applies all migrations automatically on load. To pre-create the schema manually, run the files in [configs/migrations/](configs/migrations/) in order: `psql -d admin_system -f configs/migrations/0001_initial_schema.sql` (then `0002_...`, etc.)
5. Edit [database/seed-admin.sql](database/seed-admin.sql) with your SteamID64 and run it: `psql -d admin_system -f database/seed-admin.sql`
6. Restart the server (or run `!admin_reload` if it was already running)

## Commands

| Command | Permission | Description |
| --- | --- | --- |
| `!kick <target> [reason]` | Kick (`c`) | Kick a player |
| `!ban <target> <duration> [reason]` | Ban (`d`) | Ban a player |
| `!unban <steamid> [reason]` | Unban (`e`) | Remove a ban |
| `!voice_mute <target> <duration> [reason]` (aliases `!vmute`, `!mute`) | VoiceMute (`o`) | Mute voice |
| `!voice_unmute <target>` (aliases `!vunmute`, `!unmute`) | VoiceMute (`o`) | Unmute voice |
| `!text_mute <target> <duration> [reason]` (aliases `!tmute`, `!gag`) | TextMute (`p`) | Block text chat |
| `!text_unmute <target>` (aliases `!tunmute`, `!ungag`) | TextMute (`p`) | Unblock text chat |
| `!warn <target> [reason]` | Warn (`q`) | Issue a warning (auto-ban at threshold) |
| `!admin` (aliases `!a`, `!menu`) | AdminMenu (`r`) | Open admin menu |
| `!who` (alias `!players`) | Hide (`b`) | List players, prefixes, immunity |
| `!hide` | Hide (`b`) | Toggle admin stealth |
| `!cc <target>` / `!cccancel <target>` | CheatCheck (`k`) | Call / cancel a cheat check |
| `!freeze_admin <target\|steamId> [reason]` | FreezeAdmins (`a`) | Freeze another admin's privileges |
| `!unfreeze_admin <steamId\|name>` | FreezeAdmins (`a`) | Restore a frozen admin's privileges |
| `!frozen_admins` | FreezeAdmins (`a`) | List currently frozen admins |
| `!admin_reload` (alias `!reload_admins`) | Root (`z`) | Reload admins/groups/grants/freezes from DB |

**Target Selectors:** `@all`, `@me`, `@!me`, `@t`, `@ct`, `@spec`, `@dead`, `@alive`, `@bot`, `@human`, `@random`, `@randomt`, `@randomct`, `#slot`, a SteamID (64 / `STEAM_` / `[U:1:...]`), or a name (exact, then prefix, then substring)

**Duration Format:** `30s`, `5m`, `2h`, `7d`, `1w`; a bare number means minutes; `0` or `perm` = permanent

## Configuration

Runtime configuration lives in [configs/settings.jsonc](configs/settings.jsonc) (server identity, database, punishments, abuse protection, chat, cheat-check). Admin groups (with their chat prefix and colors) and individual admins live in the `admin_groups` and `admins` PostgreSQL tables; per-server group grants live in `admin_server_groups`, and every admin action is audited in `admin_activity` -- see [configs/migrations/](configs/migrations/). Run `!admin_reload` after editing those tables to refresh in-memory state without restarting.

## Multi-Server Setup

Several game servers can share one PostgreSQL database. Each server declares a stable identity in `settings.jsonc`:

```jsonc
"server": {
  "tag": "server-1",        // unique per server; never change once grants reference it
  "name": "My Community #1" // human-readable, shown in the servers registry table
}
```

On boot the server registers itself in the `servers` table and heartbeats `last_seen` every minute, so you can see which servers are alive with a simple query.

Admin rights resolve per server:

- `admins.groups` (the array on the admin row) is **global** - it applies on every server. Use it for network-wide admins.
- `admin_server_groups(admin_steam_id, server_tag, group_name)` grants **additional groups on one server only**. The same person can be `super_admin` on `server-1` and only `moderator` on `server-2`.
- `admins.flags` and `admins.immunity` remain global; group flags/immunity apply wherever the group applies.

Punishments (bans, mutes, warnings) are always **network-wide** - a ban issued on one server applies everywhere.

Example per-server grant (also shown commented in `seed-admin.sql`):

```sql
INSERT INTO admin_server_groups (admin_steam_id, server_tag, group_name)
VALUES (76561198000000000, 'server-1', 'super_admin')
ON CONFLICT (admin_steam_id, server_tag, group_name) DO NOTHING;
```

Run `!admin_reload` on the affected server to pick up grant changes without a restart.

## Admin Abuse Protection

Protects the community from rogue admins (e.g. a purchased admin account mass-banning players). A **frozen** admin keeps their DB rows but is denied *every* admin permission - commands, the admin menu, and all actions - on every server sharing the database, until a reviewer unfreezes them.

**Automatic freezing.** Every kick/ban/mute/warn is written to the `admin_activity` audit table. After each action the admin's totals over a sliding window are checked against thresholds in `settings.jsonc`; counting is network-wide, so hopping servers doesn't evade it. Root (`z`) admins are exempt.

```jsonc
"abuseProtection": {
  "enabled": true,      // master switch for automatic freezing
  "windowMinutes": 10,  // sliding window, counted across all servers
  "maxBans": 5,         // 0 disables a counter
  "maxKicks": 10,
  "maxMutes": 15,       // voice + text combined
  "maxWarnings": 15
}
```

**Manual freezing.** An admin holding the `a` flag can run `!freeze_admin <target> [reason]` against any admin with strictly lower immunity (self-freezing is rejected). `!frozen_admins` lists open cases; `!unfreeze_admin <steamId|name>` restores privileges after review.

**What happens on freeze:** the freeze is broadcast in chat, the frozen admin is notified (immediately if online, on connect otherwise, and within ~60 seconds on other servers), and their recent punishments stay active so the reviewer can inspect `admin_activity` / the punishment tables and revert selectively. Freezes and unfreezes are themselves recorded in `admin_activity`.

## Building

The plugin builds as part of the monorepo - see the [repository README](../../README.md) for Docker and Windows build instructions.
