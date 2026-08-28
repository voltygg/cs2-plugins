---
name: crash-triage
description: Find the cause of a CS2 server crash from its minidump - resolve the faulting address to a function and source line, walk the stack, and tell a real fault apart from a perturbed latent bug. Use for "the server crashed", "access violation", "it dies on load", "read the minidump", or any cs2.exe crash on the local or remote server.
---

# Triage a CS2 crash

CS2 writes a minidump on every access violation. Resolve its address before
re-reading plugin code; guessing which change crashed costs more.

## 1. Find the dump

Local server: `<CS2_SERVER_PATH>/game/bin/win64/*.mdmp` (`CS2_SERVER_PATH` is in
`.env`). stderr names it too: `Wrote minidump to .\cs2_....mdmp`.

```powershell
Get-ChildItem "$env:CS2_SERVER_PATH\game\bin\win64" -Filter *.mdmp | Sort-Object LastWriteTime -Descending | Select-Object -First 3 Name, LastWriteTime
```

Remote instance: the dump is inside the container, `docker cp` it out first (see
`rcon-debug` for reaching the box).

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

`llvm-symbolizer` ships with the MSVC toolset; the DLL has to be the one that
crashed, so check mtimes and reproduce if the build has moved on since - a
rebuilt DLL returns a *wrong* function, not an error.

```powershell
$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath
$sym = Get-ChildItem "$vs\VC\Tools\MSVC\*\bin\Hostx64\x64\llvm-symbolizer.exe" | Select-Object -Last 1
$dll = "build\windows-msvc-release\plugins\<plugin>\windows-x86_64\<plugin>.dll"
$base = 0x180000000     # the PE image base; read it from the header if in doubt
& $sym --obj=$dll ("0x{0:x}" -f ($base + 0x1df274))
```

Without the image base you get `??:0:0`. dbghelp's `SymFromAddr` does not work
here (`ERROR_MOD_NOT_FOUND`); stay with llvm-symbolizer.

## 4. Walk the stack (when the top frame isn't enough)

```powershell
uv run python .claude/skills/crash-triage/scripts/stack.py <dump> <plugin>
```

A scan, not an unwind: expect dead frames. Release builds also fold identical
functions (`/OPT:ICF`), so corroborate a name before believing it.

## 5. Real fault, or perturbed latent bug?

A crash inside `std::_Hash`, `std::string` or an allocator means something
already corrupted memory.

If it appeared after changing a header under `include/VoltMod/`: `VoltMod::Runtime`
holds services by value, so a new member shifts everything after it, and this
codebase has had a latent out-of-bounds write that only turns fatal at some
`sizeof(Runtime)` values (+8 bytes on one service crashed admin-system on every
load, in an unrelated hash lookup).

1. `git stash push -- <header> <impl>`, rebuild, run. Crash gone means the size
   is the trigger, not your logic.
2. Rule out a stale link, which looks identical: print `sizeof(Runtime)` from
   `Runtime::Start` (framework) and from the plugin's `OnLoad`. Matching numbers
   mean staleness is not the cause; differing ones mean wipe `build/<preset>` and
   relink (see `build-local`).
3. Keep the class the same size, or find the writer - it is a real bug either way.

Otherwise it is an ordinary fault (bad gamedata offset, stale pointer, missing
null check) and step 3 already located it.

## Reproducing

Load crashes reproduce on restart. `rcon-debug` describes a bots-only server that
exercises hooks with nobody connected. Redirected stdout is buffered, so the last
log line is not the crash point.

## Report

Module, RVA, resolved function and source line, and what was read or written.
State whether the change under test causes the crash or merely exposes it - or
say you couldn't tell, rather than naming the top frame as the cause.
