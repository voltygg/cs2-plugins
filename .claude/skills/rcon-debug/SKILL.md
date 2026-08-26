---
name: rcon-debug
description: Drive a live CS2 server over RCON to test plugin behaviour without a deploy cycle - reads/writes convars, runs plugin commands, reads container logs. Use for "test it on the server", "check it live", "why is X not working in game", "run this on box-a", or any server-side behaviour question.
---

# Debug over RCON

Runs console commands against a live CS2 instance. Use it to confirm a
hypothesis **before** writing a speculative fix, and to verify what a deploy
shipped. Reach for it whenever in-game behaviour is reported ("the menu doesn't
show", "players can still bhop") instead of guessing and redeploying.

## Targets

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
**not** reachable directly from outside the box - the tunnel is mandatory.

### Local dev server

`poe rcon` only knows inventory servers. The local server started by
`uv run poe start-server` runs with `-usercon +rcon_password <RCON_PASSWORD>`
(from `.env`) on `CS2_PORT`, so use the same client directly.

**It does not listen on `127.0.0.1`** - CS2 binds one real adapter (here the
vEthernet address), so loopback gives `ConnectionRefusedError`. Look it up:

```powershell
Get-NetTCPConnection -LocalPort 27015 | Select-Object LocalAddress,State,OwningProcess
```

```bash
uv run python -c "
from deploy.tools.rcon import RconClient
c = RconClient('172.24.64.1', 27015, '12345')   # address from above
for cmd in ['sv_hibernate_when_empty 0', 'mp_warmup_end', 'mp_restartgame 1']:
    print(c.execute(cmd))
c.close()
"
```

## First: confirm the binary is current

Most "the feature is missing" reports are a stale binary, not a bug. Check
before reading any plugin code.

```bash
uv run poe rcon "meta list"
```

`WithBuildInfo` puts the commit in the version (`1.0.0+<sha>[-dirty]`). If that
sha predates the commit that added the feature, the fix is a rebuild - nothing
in the source is wrong. The admin panel title shows the same string in-game.

Deployed binaries on the remote box, to check what CI shipped by mtime:

```text
/home/steam/cs2/deploy/instances/<instance>/bundles/addons/<plugin>/bin/linuxsteamrt64/
```

## SSH and logs (box-a)

```bash
ssh -i ~/.ssh/mehnatsevar_deploy -o IdentitiesOnly=yes -o BatchMode=yes steam@207.180.234.215
docker logs box-a-cs2-main
```

Key path from `SSH_KEY_FILE` in `deploy/secrets/servers/box-a/.env`; container
names follow `<server-id>-cs2-<instance>`. Plugin lines are prefixed `[ADMIN]`,
`[BHOP]`, etc. `docker logs` also shows live command activity, which is how you
tell whether players are on.

## Useful commands

| Command            | Use                                                |
| ------------------ | -------------------------------------------------- |
| `meta list`        | Loaded plugins and their versions                  |
| `status`           | Connected players and their SteamIDs               |
| `<convar>`         | Read a convar's current value                      |
| `<convar> <value>` | Set it                                             |
| `mp_restartgame 1` | Force a fresh round                                |
| `sv_cheats 1`      | Needed for some experiments; restore it afterwards |

**Only commands whose builder called `.Console()` or `.ConsoleOnly()` are
reachable over RCON.** A command is chat-only by default and has no console
entry point:

| Plugin       | Console commands                                            |
| ------------ | ----------------------------------------------------------- |
| bhop         | `bhop_player <steamid64> <0\|1>`, `bhop_reload`             |
| anticheat    | `anticheat_reload`, `anticheat_status`, `anticheat_dumpcmd` |
| admin-system | none - all chat-only (`!ban`, `!admin_reload`, ...)         |

So an admin-system change generally can't be triggered from RCON - verify via
convars, `meta list`, `docker logs` and the database, or add `.Console()` to the
registration if an operator entry point is genuinely wanted. A permission is
never checked on the console surface; RCON is already root.

## Rules

- **Live players may be online.** Check `status` or `docker logs` first, and
  restore any convar you flip.
- Confirm a hypothesis on the server _before_ editing code.
- `mp_restartgame`, map changes and `sv_cheats` are visible to everyone - ask
  before using them on a populated instance.
- Never print RCON passwords or SSH keys into the transcript.

## Headless local repro with bots

Bots fighting each other exercise damage, movement and per-tick hooks with no
human in the game. Start the server yourself to capture stdout - VoltMod logs to
the console only, there is no file sink:

```powershell
$log = "<scratchpad>\server.log"
$a = @("-dedicated","-usercon","+map","de_dust2","-maxplayers","16","-port","27015",
       "+game_type","0","+game_mode","0","+sv_lan","1","+rcon_password","12345",
       "+bot_join_after_player","0","+bot_quota_mode","fill","+bot_quota","10",
       "+sv_hibernate_when_empty","0")
Start-Process -FilePath "C:\cs2-server\game\bin\win64\cs2.exe" -ArgumentList $a `
  -WorkingDirectory "C:\cs2-server\game\bin\win64" `
  -RedirectStandardOutput $log -RedirectStandardError "$log.err" -PassThru
```

Then over RCON, to get a live round going:

```text
sv_hibernate_when_empty 0    # without this an empty server hibernates and bots never move
mp_warmup_end
bot_difficulty 3
mp_freezetime 3
mp_roundtime 60
mp_roundtime_defuse 60
mp_restartgame 1
```

To steer what the bots shoot with - useful when you need a specific weapon
class, e.g. scoped rifles:

```text
bot_allow_grenades 0
bot_allow_pistols 0
bot_allow_shotguns 0
bot_allow_sub_machine_guns 0
bot_allow_machine_guns 0
bot_allow_rifles 0
bot_allow_snipers 1
mp_free_armor 1
```

Two traps:

- **Redirected stdout is block-buffered**, so the tail is lost on a crash and
  the last log line is not the crash point. For output that must survive, write
  it from the plugin with `fopen`/`fprintf`/`fflush` to an absolute path.
- **PowerShell blocks `Start-Sleep` followed by more commands.** Poll in one
  script (`while ($n -lt 30 -and -not (Select-String ... -Quiet)) { Start-Sleep 4; $n++ }`)
  or use Bash `run_in_background` with an `until` loop.

## Probing a path you can't reach from RCON

admin-system is chat-only, so a toggle behind `!admin` can't be flipped from the
console. Don't script the menu - hard-code the effect on an unconditional
trigger, install, watch, then remove it.

```cpp
// PROBE: every chest hit, regardless of toggles.
if (view.Hitbox == VoltMod::HitGroup::Chest)
    view.Suppress = true;
```

## Confirm the effect, not the mechanism

Log the state you mean to change and read consecutive lines. A correct-looking
mechanism can be a total no-op - `MRES_SUPERCEDE` plus writes to
`CTakeDamageInfo` and `CTakeDamageResult` changed nothing, visible only because
the probe printed hp per hit:

```text
victim=1 hp=80 hitgroup=2 dmg=11.63   <- "suppressed"
victim=1 hp=68 hitgroup=2 dmg=11.64   <- landed anyway
```

## Report

Quote actual RCON responses, name the instance, and state any convar you changed
and restored. For a local repro, say how many events you observed - two samples
is not a result.
