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
| `maps` | Maps admins may switch to |
| `weapons` | Weapons the Control > Give weapon menu offers |

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

### Map list

`maps.cycle` is the list of maps admins may switch to. The engine exposes no
usable list of its own, so this is the only source.

```jsonc
"cycle": [
  { "name": "de_dust2", "displayName": "Dust II" },
  // A non-zero workshopId addresses a workshop map by published-file id.
  { "name": "surf_utopia", "workshopId": 3070563536 }
]
```

`displayName` is the label the Map menu shows; it falls back to `name`.

Map control is menu-only: the Map category lists the cycle, and each entry
offers Change map (with a confirmation, since it ends everyone's round), Set as
next map, and Put to vote. Change map and Set as next map need the `m` flag,
Put to vote and Cancel running vote the `v` flag; either one opens the category.

Plain map names are checked against the engine at load, and one it cannot load
is logged there rather than failing when an admin picks it. Workshop maps are
not checked, because they are addressed by id and are not mounted yet.

### Weapon list

`weapons.menu` is what Control > Give weapon offers. `item` is the entity
classname; an entry not starting with `weapon_` is skipped, since it would
otherwise reach the engine as an arbitrary entity. `name` is the menu label and
falls back to `item`.

```jsonc
"menu": [
  { "name": "AK-47", "item": "weapon_ak47" }
]
```

The same menu offers a random pick from the list and a Strip weapons entry.
All three need the `k` flag.

Giving a weapon the target's team cannot buy works: the server retries once with
the pawn briefly flipped to the other team, then puts it back. A refusal the
retry cannot fix is reported to the admin who clicked.

### Fun Mode

Fun Mode is a set of server-wide round modifiers, toggled from the Fun Mode
menu. Each entry shows its current state, and Clear all turns everything off.
The category needs the `g` flag.

| Modifier | Effect |
| --- | --- |
| Low gravity | Drops `sv_gravity` for everyone |
| Headshot only | Only head hits deal damage |
| Knife round | Strips weapons and gives a knife on spawn |
| No-scope only | Scoped shots deal no damage |
| One-hit kill | Any surviving hit kills outright |

Damage from the world - falling, fire, the bomb - is never suppressed by the
aim rules, so a headshot-only round does not leave players immortal to all but
bullets. A suppressing rule beats one-hit kill: a shot that cannot land is not
amplified either.

The three damage toggles need the `OnTakeDamage_Alive` hook. If it fails to
resolve after a CS2 update, the plugin logs that at load and those three go
inert; the rest still work. `admin_status` shows whether it installed.

Low gravity restores the server's own value rather than a stock one. Turning it
on snapshots the live `sv_gravity`, and turning it off - or Clear all, or
unloading the plugin - puts that value back, so a server running
`sv_gravity 600` keeps it. A server that never enables low gravity is never
written to at all.

### Map vote

`maps.vote` controls the yes/no vote an admin opens from Map > Put to vote.
Players answer through the game's own vote panel, so the plugin keeps no tally
of its own - the engine collects the ballots.

```jsonc
"vote": {
  // Share of the ballots cast that must be yes; a majority of it is required.
  "successRatio": 0.6,
  // How long the panel stays open.
  "durationSec": 20
}
```

Judged on the ballots actually cast, not on everyone connected, so abstaining is
not a no. A passing vote queues the map for the end of the round rather than
cutting the round short. Cancel running vote calls a vote off early. Only one
vote runs at a time, and both entries need the `v` flag.

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
| `m` | Change map and queue the next map (Map menu) |
| `k` | Give and strip weapons |
| `g` | Fun Mode round modifiers |
| `v` | Start and cancel map votes (Map menu) |
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
