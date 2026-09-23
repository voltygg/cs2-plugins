---
name: crash-triage
description: Find the cause of a CS2 server crash from its minidump - resolve the fault and stack to source lines, and tell a real fault apart from a perturbed latent bug. Use for "the server crashed", "access violation", "it dies on load", "read the minidump", or any cs2.exe crash on the local or remote server.
---

# Triage a CS2 crash

Resolve the dump before re-reading plugin code.

## 1. Resolve the dump

Local dumps are `<CS2_SERVER_PATH>/game/bin/win64/*.mdmp` (newest last by name); remote ones sit
inside the container, `docker cp` them out (`rcon-debug` for reaching the box).

```bash
uv run python .claude/skills/crash-triage/scripts/mdmp.py <dump> [--module <name>] [--dll <file>]
```

It prints the exception, the faulting module+RVA resolved to function and line, then return
addresses scanned from the faulting thread's stack (`--module` picks another module). Reading it:

- It symbolizes the DLL at the path the dump names, with the PDB installed beside it. A `changed
  after the crash` warning means the lines are for a newer build: reproduce, or pass the matching
  build with `--dll`.
- `?` means no symbols: a game module, or a file that moved (`--dll`).
- A wild access address (`0x5a1183280008`) is a garbage pointer, not a null one.
- The stack is a scan, not an unwind: expect dead frames. `/OPT:ICF` folds identical functions, so
  corroborate a name before believing it.

## 2. Real fault or perturbed latent bug?

A crash in `std::_Hash`, `std::string` or the allocator means memory was already corrupt.

If it appeared after changing a header under `include/VoltMod/`: `VoltMod::Runtime` holds services
by value, so a new member shifts everything after it, and a latent out-of-bounds write has turned
fatal at some `sizeof(Runtime)` values before (+8 bytes crashed admin-system in a hash lookup).

1. `git stash push -- <header> <impl>`, rebuild, rerun. Crash gone: the size is the trigger.
2. Rule out a stale link, which looks identical: print `sizeof(Runtime)` in `Runtime::Initialize`
   and in the plugin's `OnLoad`. Different numbers: wipe `build/<preset>` and relink.
3. Keep the class the same size or find the writer; it is a real bug either way.

Otherwise it is an ordinary fault (bad gamedata offset, stale pointer, missing null check) and
step 1 located it.

## Reproducing

Load crashes reproduce on restart. `rcon-debug` has a bots-only setup that drives hooks with
nobody connected. A hung server writes no dump: take one with dbghelp's `MiniDumpWriteDump`
(`Windows Kits\10\Debuggers\x64\dbghelp.dll`); `mdmp.py` reads only dumps with an exception record.

## Report

Module, RVA, function and line, what was read or written, and whether the change under test
causes the crash or only exposes it. If you cannot tell, say so instead of blaming the top frame.
