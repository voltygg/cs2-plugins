# Bhop grants-mode smoothness: investigation record (2026-07-05/06)

Goal: make `grants` mode feel like native `enabled` mode. It floats/jerks because the
granted client predicts auto-hop (replicated convars) while the server never jumps.
Three server-side approaches were built, deployed to box-a, and live-tested. **All were
reverted** (kit revert `041a6b3`, monorepo revert `0b0f649`); the deployable code lives in
git history for cherry-picking (kit `56add29..cc69276`, monorepo `7237e09..d408749`).

## Proven facts (from live experiments on box-a, not theory)

1. **Raw convar writes DO work when held permanently.** `bhop_flip_hold 1` (raw
   `CVValue_t` writes, no callbacks/replication) makes the server auto-hop, and the
   granted client feels perfectly smooth. Console queries see the flipped values.
2. **Per-player scoped flips are NEVER observed by the jump code**, no matter which
   function brackets them. Verified with live counters (`bhop_debug`), all with the
   player's slot resolved correctly (unresolved=0, grantedSlots=1):
   - `CCSPlayer_MovementServices::ProcessMovement` detour: fires only ~150 times in
     3 min (~0.8/s) - it is NOT the per-tick movement entry; near-useless.
   - `CCSPlayerController::ProcessUsercmds` detour: fires per tick (~64/s), flip
     engaged every scope -> still floats.
   - `CPlayer_MovementServices::RunCommand` vtable hook (index win 22 / linux 23,
     verified current in SwiftlyS2 2026-06): fires per tick (~32/s), disjoint from
     ProcessUsercmds (scopes = usercmds + runcmds, no nesting) -> still floats.
3. Conclusion: **the subtick jump code samples `sv_autobunnyhopping` outside every
   per-player movement scope** (per-frame or at engine-side usercmd decode). Convar
   flipping per player is a dead end for the jump. (The old RunCommand+FlipRaw design
   failed for a different, second reason: `RawConVar` wrote only the slot -1 storage;
   the dual-slot write fix is in the reverted kit commits.)

## Leads for the next attempt (in order of promise)

1. **ModernJump state manipulation (convar-free, per-player).** CS2 split jump state
   into `CCSPlayer_MovementServices::m_ModernJump` (`CCSPlayerModernJump`: 
   LastActualJumpPressTick/Frac, **LastUsableJumpPressTick/Frac**, LastLandedTick/Frac,
   LastLandedVelocity XYZ) and `m_LegacyJump` (`CCSPlayerLegacyJump`: OldJumpPressed,
   JumpPressedTime). Auto-hop is plausibly "held button keeps the usable press fresh".
   Plan: each tick (RunCommand pre bracket), for granted players holding IN_JUMP,
   refresh `LastUsableJumpPressTick/Frac` to the current tick (and/or clear
   `LegacyJump.OldJumpPressed` - the classic cs2kz trick - as legacy-path fallback).
   The engine's own jump then fires natively on landing. Caveats: exact schema field
   names must come from a schema dump (SwiftlyS2 generated bindings use hashed keys;
   interface property names are in
   `references/swiftlys2/managed/src/SwiftlyS2.Generated/Schemas/Interfaces/CCSPlayerModernJump.cs`);
   semantics of "usable" need verifying (disassemble `CheckJumpButtonModern`).
2. **Find the real read site.** SwiftlyS2 gamedata has signatures for
   `CCSPlayer_MovementServices::CheckJumpButtonModern` / `CheckJumpButtonLegacy` /
   `OnJumpModern` / `OnJumpLegacy` (see their signatures.jsonc, with locator comments).
   Disassembling around CheckJumpButtonModern in the live libserver.so would show where
   sv_autobunnyhopping's value comes from (cached member? global sample?).
3. **Engine-side decode hook**: there may be a second, engine2-side
   `CServerSideClient::ProcessUsercmds` that decodes CLCMsg_Move at packet receive -
   if the press "usability" is decided there, that's the bracket that was missing.

## Reusable assets in the reverted commits

- Kit `56add29`: SafetyHook v0.7.0 amalgamation vendored (`vendor/safetyhook/`, builds
  as separate CMake target because the SDK's MSVC `/TP` interface flag must not touch
  Zydis.c), `InstallDetour` helper, gamedata `ProcessMovement`/`ProcessUsercmds`
  signatures (both verified unique against live linux libserver.so 2026-07-06), dual-slot
  `RawConVar` writes.
- Kit `cc69276`: three-bracket MovementHook with outermost-scope pre/post semantics and
  `GetStats()` counters.
- Monorepo `db3a25d`/`d408749`: `bhop_flip_hold` diagnostic, `bhop_debug` counter dump.
- Byte-pattern verification against the live server binary without deploying:
  `ssh box-a` then a small python re.finditer scan over
  `/home/steam/cs2/server/game/csgo/bin/linuxsteamrt64/libserver.so`.

## Ops notes

- Deploys trigger on push to the **prod branch** (`git push origin main:prod`), not main.
  CI (`ci.yml`) gates clang-format - run `uv run poe format` before committing kit code.
- The user's test SteamID: 76561198153558892. Grant via
  `uv run poe rcon "bhop_player 76561198153558892 1"`.
- Server frame spikes (~3k "Long frame"/24h, 14-19ms sim, SteamNetworkingSockets thread
  starvation) predate these changes; zero CPU steal, mostly idle 4-core EPYC 2.8GHz VPS -
  weak single-thread clock is the likely cause, not the plugins.
- As of this note, origin/prod still has the floating build `d408749` deployed; the
  revert `0b0f649` is on main and on the local prod branch but NOT pushed to
  origin/prod - push it (deploy) to restore the old jerky-but-working ForceAutoHop
  behavior.
