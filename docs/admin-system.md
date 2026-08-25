# Admin system

Deployer-facing reference for the admin-system plugin: permission flags, the
database it owns, and what changes when several servers share one database. For
the full command and menu listing, see the
[plugin README](../plugins/admin-system/README.md). For build and deploy
mechanics, see [Local development](local-development.md) and
[Deployment scripts](deployment-scripts.md).

## Configuration

Runtime settings live in `addons/admin-system/configs/settings.jsonc`. The file
is JSONC, so comments are allowed. It is validated against
`settings.schema.json`, which sets `additionalProperties: false` throughout: an
unrecognized key fails the load rather than being ignored.

| Section | Purpose |
| --- | --- |
| `plugin` | Translation file to use, without `.json` |
| `server` | This server's `tag` and display `name` in the shared database |
| `database` | PostgreSQL host, credentials, and libpq `sslMode` |
| `punishments` | Ban defaults, warning threshold, appeal notice, presets |
| `abuseProtection` | Sliding-window thresholds that auto-freeze an admin |
| `chat` | Punishment broadcasts and admin chat tagging |
| `reports` | Player report reasons, cooldowns, and duplicate suppression |
| `cheatCheck` | Cheat-check mode and the link or room API behind it |

A mistyped value fails the whole load; a malformed entry inside a list
(a punishment template, a report reason) is logged and skipped so one typo
cannot take moderation offline.

### Ban appeal notice

`punishments.appeal` controls what a banned player reads on the disconnect
screen. Both the connect-time reject and the kick after an online ban use it,
so a player sees the same text either way.

```jsonc
"appeal": {
  // `{steamId}` is substituted; empty omits the appeal from the notice.
  "url": "https://example.com/appeal?steam={steamId}",
  // Append how long the ban still has to run.
  "showExpiry": true
}
```

The notice joins the reason, the expiry, and the appeal link, dropping whatever
is not configured. With neither option set it is just the reason. The wording
around the link comes from the `kickNotice` group in
`configs/translations/<locale>.json`.

## Permission flags

Flags are single characters stored in `admins.flags` and `admin_groups.flags`.
An admin holds the union of their own flags and those of every group granted to
them. Root (`z`) grants everything.

| Flag | Access |
| --- | --- |
| `a` | Freeze and unfreeze admins |
| `b` | Hide and player-list commands |
| `c` | Kick |
| `d` | Ban |
| `e` | Unban |
| `o` | Voice mute, text mute, and warnings |
| `s` | Player controls and cheat checks |
| `f` | Fun effects: ghost, disco, smite, and size |
| `h` | Health, armor, and godmode |
| `w` | Wallhack |
| `j` | Bhop grants |
| `z` | Root access |

Immunity is separate from flags: an admin cannot act on a target whose immunity
is higher than their own. `!admin` needs no flag, but the caller must be a
registered admin, and each menu category is still gated individually.

## Database

The plugin owns its schema and applies the migrations in
`configs/migrations/` in filename order at load. To apply them by hand, run the
files in the same order with `psql`.

| Table | Holds |
| --- | --- |
| `admins` | Admin records, flags, immunity, language, and freeze state |
| `admin_groups` | Named flag and immunity bundles |
| `admin_server_groups` | Which groups an admin holds on which server tag |
| `admin_activity` | Audit trail of every punishment an admin issued |
| `players` | Seen players, names, and IP addresses |
| `bans` | Active and lifted bans |
| `voice_mutes` | Active and lifted voice mutes |
| `text_mutes` | Active and lifted text mutes |
| `warnings` | Issued warnings feeding the escalation threshold |
| `servers` | Registered server tags and display names |
| `player_reports` | Reports awaiting an external moderation service |

Gameplay decisions read in-memory caches, not the database, and the writes ride
an async worker. Editing rows directly therefore has no effect until the caches
are rebuilt, so run `!admin_reload` afterwards.

## Multiple servers on one database

Several servers may share a database. `server.tag` is the stable per-server
identity that keeps them apart, so it must be unique and must not change once
grants reference it.

- Punishments are network-wide. A ban issued anywhere applies everywhere.
- Admin grants are per-server through `admin_server_groups`, keyed by
  `server.tag`. An admin can be root on one server and unprivileged on another.
- Abuse-protection windows and admin freezes are network-wide, so an admin
  frozen on one server is frozen on all of them.

Give each server its own tag before its first start. Changing a tag later
orphans every grant that referenced the old one.

## Troubleshooting

### Admin changes made in SQL have no effect

Gameplay reads caches. Run `!admin_reload`, or restart the server.

### The plugin loads but no commands work

Check the load report in the server console and run `admin_status`. A failed
`Database` stage skips the `Admins` stage, leaving nobody holding any flag.

### A banned player sees only the reason

`punishments.appeal.url` is empty and `showExpiry` is off, so there is nothing
else to join. Set either one.

### Grants disappeared after a config change

`server.tag` changed. Restore the previous tag, or repoint the rows in
`admin_server_groups` at the new one.
