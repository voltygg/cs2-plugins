---
name: rcon-debug
description: Drive a live CS2 server over RCON to test plugin behaviour without a deploy cycle - reads/writes convars, runs plugin commands, reads container logs. Use for "test it on the server", "check it live", "why is X not working in game", "run this on box-a", or any server-side behaviour question.
---

# Debug over RCON

Runs console commands against a live CS2 instance from this machine. Use it to
confirm a hypothesis about server-side behaviour **before** writing a
speculative fix, and to verify what a deploy actually shipped.

## When to use

The user reports in-game behaviour ("the menu doesn't show", "the convar is
ignored", "players can still bhop"), or asks to check something on the server.
Reach for this instead of guessing at a fix and redeploying.

## Targets

There are two, and they use different paths.

### Remote instance from `deploy/inventory.yml`

```bash
uv run poe rcon "<command>" [--server box-a] [--instance main]
```

Both flags are optional when the inventory has exactly one server / one
instance. Pass multiple commands as separate quoted arguments; each response is
printed under a `### <command>` header.

`deploy/tools/rcon.py` resolves the host and port from `deploy/inventory.yml`,
reads `RCON_<instance>` from `deploy/secrets/servers/<server-id>/.env`, opens a
short-lived SSH tunnel, and speaks Source RCON over it. The RCON TCP port is
**not** reachable directly from outside the box — the tunnel is mandatory.

### Local dev server

`poe rcon` only knows inventory servers. The local server started by
`uv run poe start-server` runs with `-usercon +rcon_password <RCON_PASSWORD>`
(from `.env`) on `CS2_PORT`, so talk to it with the same client directly:

```bash
uv run python -c "
from deploy.tools.rcon import RconClient
c = RconClient('127.0.0.1', 27015, 'yourpassword')
print(c.execute('meta list'))
c.close()
"
```

## First: confirm the binary is current

Most 'the feature is missing' reports are a stale binary, not a bug. Check
before reading any plugin code.

```bash
uv run poe rcon "meta list"
```

Plugins are built with `WithBuildInfo`, so the reported version carries the
commit: `1.0.0+<sha>[-dirty]`. Compare that sha against `git log --oneline` — if
it predates the commit that added the feature, the fix is a rebuild and
redeploy, and nothing in the source is wrong. The admin panel title renders the
same string in-game.

On the remote box, deployed binaries live at:

```text
/home/steam/cs2/deploy/instances/<instance>/bundles/addons/<plugin>/bin/linuxsteamrt64/
```

Check their mtime to confirm what CI actually shipped.

## SSH and logs (box-a)

```bash
ssh -i ~/.ssh/mehnatsevar_deploy -o IdentitiesOnly=yes -o BatchMode=yes steam@207.180.234.215
docker logs box-a-cs2-main
```

The key path comes from `SSH_KEY_FILE` in `deploy/secrets/servers/box-a/.env`.
Container names follow `<server-id>-cs2-<instance>`. Plugin log lines are
prefixed `[ADMIN]`, `[BHOP]`, and so on. `docker logs` also shows live command
activity, which is how you tell whether players are on.

## Useful commands

| Command | Use |
| --- | --- |
| `meta list` | Loaded plugins and their versions |
| `status` | Connected players and their SteamIDs |
| `<convar>` | Read a convar's current value |
| `<convar> <value>` | Set it |
| `mp_restartgame 1` | Force a fresh round |
| `sv_cheats 1` | Needed for some experiments; restore it afterwards |

**Only commands registered with `.Surfaces = Surface::Console` are reachable
over RCON.** `CommandSpec::Surfaces` defaults to `Surface::Chat`, and chat
commands are invoked by a player, so they have no console entry point. Today
that means:

| Plugin | Console commands |
| --- | --- |
| bhop | `bhop_player <steamid64> <0\|1>`, `bhop_reload` |
| anticheat | `anticheat_reload`, `anticheat_status`, `anticheat_dumpcmd` |
| admin-system | none — every command is chat-only (`!ban`, `!admin_reload`, ...) |

So an admin-system change generally cannot be triggered from RCON. Verify it
through convars, `meta list`, `docker logs`, and the database instead, or add
`.Surfaces = Surface::Console` to the spec if an operator entry point is
genuinely wanted. Note that `Permission` is never checked on the console
surface — RCON is already root.

## Rules

- **Live players may be online.** Check `status` or `docker logs` first. Any
  convar you flip for an experiment gets restored before you finish.
- Read logs and drive the server to confirm a hypothesis *before* editing code.
  A week of blind deploy-and-guess cycles on the bhop convar bug collapsed into
  ~15 minutes of live RCON experiments once this was used.
- `mp_restartgame`, map changes, and `sv_cheats` are visible to everyone on the
  server. Ask before using them on a populated instance.
- Never print RCON passwords or SSH keys into the transcript.

## Headless local repro

To exercise per-tick hooks with no human player, run the local server with bots:

```powershell
cs2.exe -dedicated -console -usercon +map de_dust2 +game_type 0 +game_mode 0 +sv_lan 1 +bot_join_after_player 0 +bot_quota_mode fill +bot_quota 6
```

`bot_join_after_player 0` is what makes bots join an empty server.

## Report

Quote the actual RCON responses rather than summarising them, name the instance
you touched, and state any convar you changed and restored.
