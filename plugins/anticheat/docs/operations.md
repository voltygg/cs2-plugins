# Operations

## Rollout

`anticheat` is deployed to `panel-a` in
[`deploy/inventory.yml`](../../../deploy/inventory.yml) with no setting
overrides, so the shipped `settings.jsonc` is what reaches the server.

1. Run in `observe` mode with a webhook and collect at least one week of
   populated traffic.
2. Investigate suspicious evidence with `anticheat_dumpcmd`. Disable any
   detector that produces false positives.
3. Switch to `alert` for a second soak period.
4. Enable `ban` one detector at a time. Start with `invalidCvar`, then
   `namechanger` and `dllInjection`. Add `silentAim`, `antiAim`, `aimbot`,
   `recoil`, `triggerbot`, `aimAssist`, `wallhack` and `aimlock` last, with
   a soak between each change.

## Maintenance after CS2 updates

Revalidate these game-data entries after every game update. A stale entry may
crash, create false evidence, or silently disable detection.

| Entry | Failure mode |
| --- | --- |
| `CPlayer_MovementServices::RunCommand` | Crash on the first movement tick, unless the vtable slot check catches it |
| `CUserCmd::CSGOUserCmdPB` | Missing values silence aim modules; stale values can resemble valid angles |
| `CUserCmdBase::cmdNum` | Command chains collapse, silently disabling aimbot and part of antiaim |
| `CBaseEntity::Teleport` | Teleport grace stops suppressing discontinuities, so false positives appear |
| `CServerSideClient::ProcessRespondCvarValue` | The vtable slot stops holding code, so `Hooks.ClientConVars.Available()` fails |
| `CServerSideClientBase::m_nClientSlot` | A drifted offset still binds, so responses can reach the wrong player |
| `TraceShape` | The pattern stops matching, `World.Trace.Available()` fails, and the wallhack rule goes inert |
| `CCSPlayerPawn::m_iLastWeaponFireUsercmd`, `m_iShotsFired`, `m_bIsScoped`, `CCSPlayer_AimPunchServices::*` | Schema drift aborts the framework load; regenerate with `voltmod schemagen` |

The entries live in the framework's `gamedata/gamedata.jsonc`; its guide has the
re-verification procedure. `anticheat_status` exposes `teleportTracker` from
`Hooks.Teleport.Available()`, `sightLines` from `World.Trace.Available()`, and
reports client convars as `degraded` when `Hooks.ClientConVars.Available()`
fails. In that state, network polling stops and `invalid_cvar` uses userinfo
only. Sight lines read `the engine has not traced yet this map` until the
first engine trace after a map start, which happens as soon as anything moves.
