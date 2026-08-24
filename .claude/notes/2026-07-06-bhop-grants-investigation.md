# Bhop grants-mode investigation (2026-07-05/06)

Status: resolved by enforcing granted hops on the server after movement simulation.
Replicated convar overrides still give the client matching prediction.

## Findings

- Permanent raw `sv_autobunnyhopping` writes made server and client movement agree.
- Per-player convar flips did not affect the subtick jump decision, even when scoped
  around `ProcessMovement`, `ProcessUsercmds`, or `RunCommand`.
- `ProcessMovement` was not the per-tick entry point. `ProcessUsercmds` and
  `RunCommand` fired per tick, but their scoped values were still too late for the
  engine's jump decision.
- Pre-simulation hop enforcement can miss a landing that occurs in the same command.
  Post-simulation enforcement sees the resulting ground state.

The implementation therefore keeps grants session-scoped, replicates the prediction
values to granted clients, and calls `ForceAutoHop` after simulation. Do not replace
that path with scoped convar writes unless the engine read site changes.

## Reverted experiments

The tested detour implementation remains available in Git history:

- VoltMod `56add29..cc69276`: SafetyHook integration, movement signatures,
  dual-slot raw convar writes, and hook counters.
- cs2-plugins `7237e09..d408749`: hold/flip diagnostics and counter reporting.
- Reverts: VoltMod `041a6b3`, cs2-plugins `0b0f649`.

The signatures were verified against the Linux `libserver.so` current on
2026-07-06. Reverify them before reusing the experimental hooks.
