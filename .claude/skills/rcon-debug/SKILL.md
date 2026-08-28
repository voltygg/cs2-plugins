---
name: rcon-debug
description: Drive a live CS2 server over RCON to test plugin behaviour without a deploy cycle - reads/writes convars, runs plugin commands, reads container logs. Use for "test it on the server", "check it live", "why is X not working in game", "run this on box-a", or any server-side behaviour question.
---

# Debug over RCON

Runs console commands against a live CS2 instance. Use it to confirm a hypothesis
before writing a speculative fix, and to verify what a build shipped. Reach for it
whenever in-game behaviour is reported ("the menu doesn't show", "players can
still bhop") instead of guessing and rebuilding.

## Local dev server

`uv run poe start-server` launches the server at `CS2_SERVER_PATH` on `CS2_PORT`
with `-usercon +rcon_password <RCON_PASSWORD>`, all from `.env`. Start it from
PowerShell with redirected stdout when you need the log afterwards (VoltMod logs
to the console only):

```powershell
$log = "<scratchpad>\server.log"
Start-Process -FilePath "uv" -ArgumentList "run poe start-server" -RedirectStandardOutput $log -RedirectStandardError "$log.err" -PassThru
```

A client on the same machine joins with `connect localhost:27015` in its console.
Nothing can type into the client for you, so anything that needs a human in the
game (clicking a menu, pressing keys) waits for the user; everything else below
runs headless.

Talk to it with the same client the remote path uses (`deploy/tools/rcon.py`),
reading the password from `.env` rather than pasting it:

```bash
uv run python -c "
import os
from deploy.tools.rcon import RconClient
env = dict(l.strip().split('=', 1) for l in open('.env') if '=' in l and not l.startswith('#'))
c = RconClient('127.0.0.1', int(env.get('CS2_PORT', 27015)), env['RCON_PASSWORD'])
for cmd in ['meta list', 'status']:
    print(c.execute(cmd))
c.close()
"
```

If loopback is refused, CS2 bound one adapter instead: find it with
`Get-NetTCPConnection -LocalPort <port> | Select-Object LocalAddress` and use that
address.

## Remote instance

```bash
uv run poe rcon "<command>" ["<command>" ...] [--server <id>] [--instance <name>]
```

Resolves host and port from `deploy/inventory.yml` and `RCON_<instance>` from
`deploy/secrets/servers/<id>/.env`, tunnels over SSH (the RCON port is not
exposed), and prints each response under a `### <command>` header. The flags are
optional when the inventory has one server / one instance.

Logs and dumps need the box itself. The SSH host and `SSH_KEY_FILE` are in the
same inventory and `.env`; containers are named `<server-id>-cs2-<instance>`:

```bash
ssh -i <SSH_KEY_FILE> -o IdentitiesOnly=yes -o BatchMode=yes steam@<host>
docker logs <server-id>-cs2-<instance>
```

Deployed binaries live under
`/home/steam/cs2/deploy/instances/<instance>/bundles/addons/<plugin>/`; their
mtime says what CI shipped.

## First: confirm the binary is current

Most "the feature is missing" reports are a stale binary. `meta list` shows each
plugin's version as `1.0.0+<sha>[-dirty]` (from `WithBuildInfo`); if that sha
predates the commit that added the feature, rebuild instead of reading code.

## What is reachable from the console

Only commands registered with `.Console()` or `.ConsoleOnly()`; chat commands have
no console entry point, and permissions are never checked on the console.

| Plugin | Console commands |
| --- | --- |
| bhop | `bhop_player <steamid64> <0\|1>`, `bhop_reload` |
| anticheat | `anticheat_reload`, `anticheat_status`, `anticheat_dumpcmd` |
| ui-lab | `uilab_*` - spawn/write/click-log a layout, `uilab_menu <slot>` opens the Panorama menu host for a player, `uilab_probe` reports capabilities |
| admin-system | none; everything is chat (`!admin`, `!ban`, `!admin_reload`) |

So an admin-system change is verified through convars, `meta list`, logs and the
database - or, for anything on the shared menu/UI machinery, through ui-lab, which
exercises the same framework services from the console.

Useful engine commands: `meta list`, `status`, `<convar>` (read) / `<convar>
<value>` (set), `mp_restartgame 1`, `sv_cheats 1` (restore it afterwards).

## Rules

- Live players may be online. Check `status` or the logs first, and restore any
  convar you flip.
- Confirm a hypothesis on the server before editing code.
- `mp_restartgame`, map changes and `sv_cheats` are visible to everyone; ask before
  using them on a populated instance.
- Never print RCON passwords or SSH keys into the transcript.

## Headless repro with bots

Bots exercise damage, movement and per-tick hooks with no human in the game.
Start the server as above, then over RCON:

```text
sv_hibernate_when_empty 0    # an empty server hibernates and bots never move
bot_join_after_player 0
bot_quota_mode fill
bot_quota 10
mp_warmup_end
bot_difficulty 3
mp_freezetime 3
mp_roundtime 60
mp_roundtime_defuse 60
mp_restartgame 1
```

`bot_allow_*` convars steer what they shoot with. Two traps:

- Redirected stdout is block-buffered, so the tail is lost on a crash. For output
  that must survive, write it from the plugin with `fopen`/`fprintf`/`fflush`.
- PowerShell's `Start-Sleep` is blocked between commands here; poll in one script
  or use Bash `run_in_background` with an `until` loop.

## Probing a path you can't reach from the console

Don't script a chat menu. Hard-code the effect on an unconditional trigger,
install, watch, then remove it:

```cpp
// PROBE: every chest hit, regardless of toggles.
if (view.Hitbox == VoltMod::HitGroup::Chest)
    view.Suppress = true;
```

Log the state you mean to change and read consecutive lines: a correct-looking
mechanism can be a no-op (a `SUPERCEDE` plus writes to `CTakeDamageInfo` changed
nothing, visible only because the probe printed hp per hit).

## Report

Quote actual RCON responses, name the instance, and state any convar you changed
and restored. For a local repro, say how many events you observed - two samples
is not a result.
