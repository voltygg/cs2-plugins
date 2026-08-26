"""Scan the faulting thread's stack for return addresses inside one module.

Usage: uv run python stack.py <dump.mdmp> [module-substring]

A scan, not an unwind: every slot pointing into the module is reported, so
expect dead frames. Feed the RVAs to llvm-symbolizer with the image base added.
"""

from __future__ import annotations

import struct
import sys

# Same directory, same commit: the minidump header/module parsing lives in one place so a wrong
# field offset is fixed once.
from mdmp import EXCEPTION_STREAM, MODULE_LIST_STREAM, _modules, _streams

THREAD_LIST_STREAM = 3

THREAD_ENTRY_SIZE = 48
MAX_FRAMES = 40


def main(path: str, needle: str = "admin-system") -> None:
    data = open(path, "rb").read()
    streams = _streams(data)
    mods = _modules(data, streams[MODULE_LIST_STREAM][1])

    target = next((m for m in mods if needle.lower() in m[2].lower()), None)
    if not target:
        raise SystemExit(f"no module matching {needle!r}")
    tbase, tsize, tname = target
    print(f"target: {tname} base=0x{tbase:x} size=0x{tsize:x}")

    crash_tid = struct.unpack_from("<I", data, streams[EXCEPTION_STREAM][1])[0]

    loc = streams[THREAD_LIST_STREAM][1]
    count = struct.unpack_from("<I", data, loc)[0]
    off = loc + 4
    for _ in range(count):
        entry = struct.unpack_from("<IIIIQQIIII", data, off)
        # MINIDUMP_THREAD: id, suspend, priority class, priority, teb, then the stack's
        # MEMORY_DESCRIPTOR (start va, size, rva) and the two context fields.
        tid, stack_start, stack_size, stack_rva = entry[0], entry[5], entry[6], entry[7]
        off += THREAD_ENTRY_SIZE
        if tid != crash_tid:
            continue

        print(f"thread {tid} stack 0x{stack_start:x} size 0x{stack_size:x}")
        seen, last = [], None
        for i in range(0, max(0, stack_size - 8), 8):
            val = struct.unpack_from("<Q", data, stack_rva + i)[0]
            if tbase <= val < tbase + tsize:
                rva = val - tbase
                if rva and rva != last:  # skip the base itself; collapse runs
                    seen.append(rva)
                    last = rva
        print(f"{len(seen)} candidate return addresses (RVA), innermost first:")
        for rva in seen[:MAX_FRAMES]:
            print(f"  0x{rva:x}")
        return

    print("faulting thread not present in the thread list")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    main(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else "admin-system")
