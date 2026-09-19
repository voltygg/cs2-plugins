# Admin system

A database-backed administration plugin for CS2 servers. It provides
moderation commands, WASD menus, permissions, effects, player reports,
multi-server grants, abuse protection, and cheat-check workflows.

## Features

- Kick, ban, unban, voice mute, text mute, and warning commands.
- Player controls including slay, teleport, freeze, noclip, team changes,
  health, armor, speed, size, and bury.
- Visual and gameplay effects such as ghost, disco, wallhack, smite, model
  selection, and optional [bhop grants](../bhop/README.md).
- Map control through the admin menu: change level, queue the next map, and put
  a map to the game's own yes/no vote panel.
- Weapon control through the menu: give a configured weapon, a random one, or
  strip a player.
- Fun Mode round modifiers through the menu: low gravity, headshot only, knife
  round, and one-hit kill.
- Groups, permissions, immunity, per-server grants, and admin stealth.
- Network-wide punishments and automatic punishment enforcement.
- Automatic or manual freezing of abusive admins with an audit trail.
- Player reports for an external website or moderation service to process.
- Fixed-link, website-room, and player-provided cheat-check workflows.
- Async database work with in-memory gameplay caches and automatic migrations.

## Requirements

- CS2 dedicated server
- [Metamod:Source 2.0](https://www.sourcemm.net/)
- PostgreSQL 13+, MariaDB 10.5+, or SQLite (bundled, no server needed)

## Install

1. Extract the release into the server's `game/csgo/` directory.
2. Configure `addons/voltmod/plugins/admin-system/configs/settings.jsonc`.
3. Give every server sharing the database a unique, stable `server.tag`.
4. Start the server and let the plugin apply its migrations.
5. Put your SteamID64 in [`database/seed-admin.sql`](database/seed-admin.sql),
   render it for your driver and pipe it in:

   ```bash
   uv run voltmod database sql database/seed-admin.sql --driver postgres | psql -d admin_system
   ```

6. Restart the server or run `!admin_reload`.

To apply migrations manually, render
[`configs/migrations/`](configs/migrations/) the same way.

## Permissions

Permissions are names in a JSON array, for example `'["admin.kick", "admin.ban"]'`.
`*` grants every permission and `admin.*` every `admin.` one. Self-targeting
remains available where the action supports it.

| Permission | Access |
| --- | --- |
| `admin.freeze_admins` | Freeze and unfreeze admins |
| `admin.hide` | Hide and player-list commands |
| `admin.kick` | Kick |
| `admin.ban` | Ban |
| `admin.unban` | Unban |
| `admin.mute` | Voice mute, text mute, and warnings |
| `admin.control` | Player controls and cheat checks |
| `admin.fun` | Fun effects: ghost, disco, smite, and size |
| `admin.health` | Health, armor, and godmode |
| `admin.wallhack` | Wallhack |
| `admin.bhop` | Bhop grants |
| `admin.map` | Change map and queue the next map (Map menu) |
| `admin.weapon` | Give and strip weapons (Control menu) |
| `admin.fun_mode` | Fun Mode round modifiers (Fun menu) |
| `admin.vote` | Start and cancel map votes (Map menu) |
| `*` | Root access |

`!admin` has no dedicated permission, but the caller must be a registered admin.
Individual menu categories and actions remain permission-gated.

## Commands

Map control, weapons, and Fun Mode are menu-only - `!admin` opens the panel and
each category is gated by its own permission. The commands below are the ones with no
menu equivalent.

### Moderation and administration

| Command | Permission | Purpose |
| --- | --- | --- |
| `!kick <target> [reason]` | `admin.kick` | Kick a player |
| `!ban <target> <duration> [reason]` | `admin.ban` | Ban a player |
| `!unban <steamid> [reason]` | `admin.unban` | Remove a ban |
| `!voice_mute <target> <duration> [reason]` | `admin.mute` | Mute voice; aliases: `!vmute`, `!mute` |
| `!voice_unmute <target>` | `admin.mute` | Restore voice; aliases: `!vunmute`, `!unmute` |
| `!text_mute <target> <duration> [reason]` | `admin.mute` | Block chat; aliases: `!tmute`, `!gag` |
| `!text_unmute <target>` | `admin.mute` | Restore chat; aliases: `!tunmute`, `!ungag` |
| `!warn <target> [reason]` | `admin.mute` | Warn a player and apply configured escalation |
| `!admin` | registered admin | Open the menu; aliases: `!a`, `!menu` |
| `!who` | `admin.hide` | List players, prefixes, and immunity; alias: `!players` |
| `!hide` | `admin.hide` | Toggle admin stealth |
| `!admin_reload` | `*` | Reload admins, groups, grants, and freezes; alias: `!reload_admins` |

### Admin freezes

| Command | Permission | Purpose |
| --- | --- | --- |
| `!freeze_admin <target\|steamId> [reason]` | `admin.freeze_admins` | Suspend a lower-immunity admin |
| `!unfreeze_admin <steamId\|name>` | `admin.freeze_admins` | Restore a frozen admin |
| `!frozen_admins` | `admin.freeze_admins` | List active freezes |

### Cheat checks and reports

| Command | Permission | Purpose |
| --- | --- | --- |
| `!check <target>` | `admin.control` | Start a cheat check |
| `!cccancel <target>` | `admin.control` | Cancel a check; alias: `!uncheck` |
| `!cc <link>` | none | Let the suspect submit a verification link |
| `!report` | none | Open the report menu; alias: `!r` |

`!report` takes no arguments. The menu resolves the selected player to a
SteamID, so a report cannot land on the wrong player with a similar name.

### Targets and durations

Targets may be `@all`, `@me`, `@!me`, `@t`, `@ct`, `@spec`, `@dead`,
`@alive`, `@bot`, `@human`, `@random`, `@randomt`, `@randomct`, `#slot`,
a SteamID, or a player name. Names resolve by exact match, then prefix, then
substring.

Durations accept values such as `30s`, `5m`, `2h`, `7d`, and `1w`. A bare
number means minutes; `0` and `perm` mean permanent.

## Configuration

Runtime settings live in
[`configs/settings.jsonc`](configs/settings.jsonc). The file controls server
identity, database access, punishment templates, abuse thresholds, reports,
chat, and cheat checks. The shipped defaults include:

- Russian (`ru`) as the default locale.
- A 10-minute abuse window with limits of 5 bans, 10 kicks, 15 mutes, and
  15 warnings.
- A 120-second report cooldown and 1,800-second duplicate-report window.
- Custom report reasons enabled.
- A 120-second cheat-check timeout with automatic kicking enabled.

Cheat checks support `fixedLink`, `websiteAutoRoom`, and `playerProvided`
modes. Website presence polling is optional.

Player-facing messages live under
[`configs/translations/`](configs/translations/). Keep every language file
key-parallel. Reload database-backed admin state with `!admin_reload` after
changing groups, grants, or admins.

## Multi-server setup

Several servers can share one database:

```jsonc
"server": {
  "tag": "server-1",
  "name": "My Community #1"
}
```

Treat `server.tag` as permanent once grants reference it. On startup, the
plugin registers the server and updates `last_seen` every minute.

- `admins.groups` applies network-wide.
- `admin_server_groups` adds groups for one `server.tag`.
- Admin permissions and immunity are global.
- Group permissions and immunity apply wherever that group is granted.
- Bans, mutes, and warnings apply across all servers sharing the database.

Example per-server grant:

```sql
INSERT INTO admin_server_groups (admin_steam_id, server_tag, group_name)
VALUES (76561198000000000, 'server-1', 'super_admin')
ON CONFLICT (admin_steam_id, server_tag, group_name) DO NOTHING;
```

Run `!admin_reload` on affected servers after changing grants.

## Abuse protection

Every kick, ban, mute, and warning is written to `admin_activity`. After each
action, the plugin checks the issuing admin's network-wide totals over the
configured sliding window. Root admins are exempt.

An admin with `a` can manually freeze another admin with strictly lower
immunity. Frozen admins keep their database records but lose all permissions
on every connected server until unfrozen. The plugin broadcasts the freeze,
notifies the admin immediately or on connection, and propagates the state to
other servers within about one minute. Existing punishments remain active for
review, and freeze changes are audited.

## Player reports

`!report` opens a WASD menu for player, reason, and confirmation. Players can
keep moving while it is open. Bots, the reporter, and recently reported players
are unavailable.

Cooldown and duplicate checks are enforced throughout the menu flow and again
at confirmation. A failed database write refunds the cooldown. Custom reasons
are limited to 64 characters and stored with the code `other`.

The plugin only inserts reports. An external service owns triage fields such as
`status`, `handled_by`, `handled_at`, and `resolution`.

## Database tables

The main tables are:

- `admins`, `admin_groups`, and `admin_server_groups` for access.
- `servers` for server identity and heartbeats.
- `admin_activity` for the audit trail.
- Punishment tables for network-wide enforcement.
- `player_reports` for external report triage.

For shared build commands, see the [repository README](../../README.md).
