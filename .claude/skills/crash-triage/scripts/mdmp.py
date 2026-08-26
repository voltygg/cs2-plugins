"""Print a minidump's exception record and the module that faulted.

Usage: uv run python mdmp.py <dump.mdmp>

Reads only the exception and module-list streams - no debugger, no symbols.
Feed the reported RVA to llvm-symbolizer for a source line.
"""

from __future__ import annotations

import struct
import sys

EXCEPTION_STREAM = 6
MODULE_LIST_STREAM = 4

#: sizeof(MINIDUMP_MODULE); the name RVA sits in its leading fixed fields.
MODULE_ENTRY_SIZE = 108

_PLUGIN_HINTS = ("admin", "voltmod", "metamod", "anticheat", "bhop")


def _streams(data: bytes) -> dict[int, tuple[int, int]]:
    sig, _ver, count, rva = struct.unpack_from("<4sIII", data, 0)
    if sig != b"MDMP":
        raise SystemExit(f"not a minidump: {sig!r}")
    out = {}
    for i in range(count):
        kind, size, loc = struct.unpack_from("<III", data, rva + i * 12)
        out[kind] = (size, loc)
    return out


def _modules(data: bytes, loc: int) -> list[tuple[int, int, str]]:
    count = struct.unpack_from("<I", data, loc)[0]
    off = loc + 4
    mods = []
    for _ in range(count):
        base, size, _cs, _ts, name_rva = struct.unpack_from("<QIIII", data, off)
        off += MODULE_ENTRY_SIZE
        name_len = struct.unpack_from("<I", data, name_rva)[0]
        name = data[name_rva + 4 : name_rva + 4 + name_len].decode("utf-16-le")
        mods.append((base, size, name))
    return mods


def main(path: str) -> None:
    data = open(path, "rb").read()
    streams = _streams(data)
    mods = _modules(data, streams[MODULE_LIST_STREAM][1]) if MODULE_LIST_STREAM in streams else []

    def owner(addr: int) -> tuple[str | None, int]:
        for base, size, name in mods:
            if base <= addr < base + size:
                return name, addr - base
        return None, 0

    if EXCEPTION_STREAM in streams:
        loc = streams[EXCEPTION_STREAM][1]
        tid = struct.unpack_from("<I", data, loc)[0]
        code, _flags, _rec, addr = struct.unpack_from("<IIQQ", data, loc + 8)
        nparams = struct.unpack_from("<I", data, loc + 32)[0]
        params = struct.unpack_from("<15Q", data, loc + 40)
        name, off = owner(addr)
        print(f"thread      : {tid}")
        print(f"code        : 0x{code:08x}")
        print(f"address     : 0x{addr:x}")
        print(f"module      : {name}+0x{off:x}" if name else "module      : <unknown>")
        print(f"params      : {[hex(p) for p in params[:nparams]]}")
        if code == 0xC0000005 and nparams >= 2:
            kind = {0: "read", 1: "write", 8: "execute"}.get(params[0], str(params[0]))
            print(f"              -> {kind} of 0x{params[1]:x}")

    print("\nplugin modules:")
    for base, size, name in mods:
        if any(h in name.lower() for h in _PLUGIN_HINTS):
            print(f"  {name}  base=0x{base:x} size=0x{size:x}")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    main(sys.argv[1])
