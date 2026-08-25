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
| `weapons` | Weapons `!give` and the weapon menu offer |

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

`displayName` is the menu label and is also typeable, so `!map dust` finds a map
labelled "Dust II". Queries match exact names first, then prefixes, then
substrings; one matching several maps is reported, not guessed at.

Plain map names are checked against the engine at load, and one it cannot load
is logged there rather than failing on the first `!map`. Workshop maps are not
checked, because they are addressed by id and are not mounted yet.

### Weapon list

`weapons.menu` is what `!give` and the Control > Give weapon menu offer. `item`
is the entity classname; an entry not starting with `weapon_` is skipped,
since it would otherwise reach the engine as an arbitrary entity.

```jsonc
"menu": [
  { "name": "AK-47", "item": "weapon_ak47" }
]
```

Typing the `weapon_` prefix is optional, and the display name is matched too,
so `!give ak47`, `!give weapon_ak47` and `!give AK-47` all work.

Giving a weapon the target's team cannot buy works: the server retries once with
the pawn briefly flipped to the other team, then puts it back.

### Fun Mode

Fun Mode is a set of server-wide round modifiers, toggled from the Fun Mode menu
or with `!fun <toggle>`. `!fun` with no argument lists them and their state;
`!fun off` turns everything off.

| Toggle | Effect |
| --- | --- |
| `lowgravity` | Drops `sv_gravity` for everyone |
| `headshotonly` | Only head hits deal damage |
| `kniferound` | Strips weapons and gives a knife on spawn |
| `noscopeonly` | Scoped shots deal no damage |
| `onehitkill` | Any surviving hit kills outright |
| `infinitemoney` | Tops every player back up to $16000 |
| `chickenbots` | Puts the chicken model on every bot |

Damage from the world - falling, fire, the bomb - is never suppressed by the
aim rules, so a headshot-only round does not leave players immortal to all but
bullets. A suppressing rule beats one-hit kill: a shot that cannot land is not
amplified either.

The three damage toggles need the `OnTakeDamage_Alive` hook. If it fails to
resolve after a CS2 update, the plugin logs that at load and those three go
inert; the rest still work. `admin_status` shows whether it installed.

Chicken bots puts the chicken model on the bot pawn rather than spawning a real
chicken: no server-side chicken spawn is reliable enough to build on.

### Rock the vote

`maps.rtv` controls the player-driven map change. `!rtv` takes one vote per
SteamID; once enough players agree, the next map in `maps.cycle` is queued for
the end of the round rather than changing level at once.

```jsonc
"rtv": {
  "enabled": true,
  // Share of connected humans who must agree; a majority of it is required.
  "successRatio": 0.6,
  // Seconds after a map starts before !rtv is accepted.
  "voteDelaySec": 120
}
```

Bots are excluded, so they cannot raise the bar out of reach on a mostly-empty
server, and a player who disconnects gives their vote back. The tally resets on
a map change.

`!votemap <name>` puts one map to the game's own yes/no panel and `!cancelvote`
calls it off; both need the `v` flag. The panel judges on ballots actually
cast, so abstaining is not a no.

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
| `m` | Change map and queue the next map |
| `k` | Give and strip weapons |
| `g` | Fun Mode round modifiers |
| `v` | Start and cancel map votes |
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
