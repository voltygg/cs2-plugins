---
name: crash-triage
description: Find the cause of a CS2 server crash from its minidump - resolve the faulting address to a function and source line, walk the stack, and tell a real fault apart from a perturbed latent bug. Use for "the server crashed", "access violation", "it dies on load", "read the minidump", or any cs2.exe crash on the local or remote server.
---

# Triage a CS2 crash

CS2 writes a minidump on every access violation. Resolve its address before
re-reading plugin code - guessing which change crashed costs more.

## 1. Find the dump

```powershell
Get-ChildItem "C:\cs2-server\game\bin\win64" -Filter "*.mdmp" |
  Sort-Object LastWriteTime -Descending | Select-Object -First 3 Name,LastWriteTime
```

stderr names it too (`Wrote minidump to .\cs2_....mdmp`). On box-a, `docker cp`
it out of the container first.

## 2. Read the exception record

```powershell
uv run python .claude/skills/crash-triage/scripts/mdmp.py <dump>
```

```text
code        : 0xc0000005
module      : ...\admin-system.dll+0x1df274
              -> read of 0x5a1183280008
```

A wild target address means a garbage pointer, not a null one.

## 3. Resolve the RVA

```powershell
$dll = (Get-Item "build\windows-msvc-release\plugins\admin-system\windows-x86_64\admin-system.dll").FullName
$va = "0x{0:x}" -f (0x180000000 + 0x1df274)     # image base + RVA
& "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\llvm-symbolizer.exe" --obj=$dll $va
```

- Add the image base or you get `??:0:0`. Read it from the PE header rather than
  assuming `0x180000000`.
- A rebuild since the dump silently returns a _wrong_ function, not an error.
  Check mtimes; reproduce the crash if the build has moved on.
- Don't use dbghelp - `SymFromAddr` returns `ERROR_MOD_NOT_FOUND` here.

## 4. Walk the stack (when the top frame isn't enough)

```powershell
uv run python .claude/skills/crash-triage/scripts/stack.py <dump> admin-system
```

A scan, not an unwind: expect dead frames. Release builds also fold identical
functions (`/OPT:ICF`), so corroborate a name before believing it.

## 5. Real fault, or perturbed latent bug?

A crash inside `std::_Hash`, `std::string` or an allocator means something
already corrupted memory.

**If it appeared after changing a header under `include/VoltMod/`:**
`VoltMod::Runtime` holds services **by value**, so a new member shifts
everything after it. This codebase has a latent out-of-bounds write that only
turns fatal at some `sizeof(Runtime)` values - +8 bytes on `Hooks::Damage`
crashed admin-system on every load, in an unrelated hash lookup.

1. `git stash push -- <header> <impl>`, rebuild, run. Crash gone → size is the
   trigger, not your logic.
2. Rule out an ABI mismatch, which looks identical: print `sizeof(Runtime)` and
   `offsetof(Commands)` from `Runtime::Start` (lib) and `App::Start` (plugin).
   Matching numbers mean staleness is not the cause and a clean rebuild won't
   help.
3. Keep the class the same size - put the new state in a file-scope variable in
   the `.cpp`, set once at load, with a comment saying why.

Otherwise it's an ordinary fault (bad gamedata offset, stale pointer, missing
null check) and step 3 already located it.

## Reproducing

Load crashes reproduce on restart; see `rcon-debug` for the bots-only setup.
Redirected stdout is buffered, so the last log line is **not** the crash point.

## Report

Module, RVA, resolved function and source line, and what was read or written.
State whether the change under test _causes_ the crash or merely _exposes_ it -
or say you couldn't tell, rather than naming the top frame as the cause.
