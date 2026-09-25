# Stronghold placement preview delay (2026-09-25)

Status: worked around. The shop preloads every structure model when it opens, and the red/green
placement ghost now shows after a second or two instead of a long wait. The proper fix is still
open.

## Findings

- On the prod test server (panel-a, real players), the placement ghost sometimes showed at once,
  sometimes only after a long delay. Placing the structure with E showed it normally.
- A client loads a model when the first entity wearing it arrives, and can release it again once
  no entity uses it. The ghost waits for that load.
- VoltMod's `runtime.Precache` adds all 62 Stronghold resources to the session manifest at map
  load (`Precache: added 62 resource(s)` in the server log). That makes the server spawn the
  models, but it evidently does not keep them loaded on clients: without a client-side preload the
  delay was there, and preloading from the shop shortened it. Not confirmed in a client.
- The old `ModelPreload` ran once per map, at a player's first spawn, with props at their feet for
  8 seconds. Later placements still waited, since the client had let the models go by then.

## What is in place

- `plugins/stronghold/src/World/ModelPreload.*`: `Preload(slot)` spawns one near-invisible
  (alpha 2) prop per structure model 48 units in front of the player's eyes, sent only to them,
  removed after 30 seconds. It skips a player whose last preload is still up.
- `Shop::Open` calls it, and so does the first spawn of each map.
- Offered but not done: also preloading on every respawn. That is more of the same workaround,
  and costs about 38 hidden props per player for 30 seconds after each spawn.

## Proper fix: load the models with the map

Clients load a spawn group's resources during the loading screen and keep them for the map.
Every CS2 map already loads extra prefab spawn groups (`status` on panel-a lists
`prefabs/misc/team_select`, `end_of_match`, the team intros). So:

1. Build a small prefab map with one hidden `prop_dynamic` per structure model, compile it into
   the meatgg workshop addon, and put the compiled file in `server-assets` too (the server mounts
   no workshop addon).
2. Have VoltMod load that prefab as a spawn group at map start, as the game loads its own.
3. Delete `ModelPreload` and the `Shop` → `ModelPreload` dependency.

Costs: a new engine binding in VoltMod to load a spawn group (gamedata on Windows and Linux,
re-checked after CS2 updates); a Hammer prefab; it likely takes effect only from the next map
load; a VoltMod release.

## Cheaper check first

Before building prefab support, confirm the session manifest never reaches clients, rather than
VoltMod adding resources at the wrong moment. If it's the timing, fixing that is the proper fix
and much smaller. Test in a client: disable the shop preload, join, spawn, open the shop right
away and pick a structure you have not seen yet this map. If the ghost still waits, the manifest
is server-only and the prefab route stands.

Related: VoltMod `docs/sdk/entities.md` (Precache), `src/Engine/Server/Precache.cpp`, and the
host hook on `CGameRulesGameSystem`'s `OnBuildGameSessionManifest`.
