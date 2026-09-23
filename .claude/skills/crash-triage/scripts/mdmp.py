"""Print a minidump's fault and the faulting thread's return addresses, resolved to source lines.

Usage: uv run python mdmp.py <dump.mdmp> [--module <name>] [--dll <file>]

--module picks the module whose frames to list; default: the one that faulted. Frames come from a
stack scan, not an unwind, so expect dead ones. Lines resolve through llvm-symbolizer against the
module file the dump names (the install copies its PDB beside it), or against --dll.
"""

import argparse
import struct
import subprocess
from pathlib import Path

THREAD_LIST_STREAM = 3
MODULE_LIST_STREAM = 4
EXCEPTION_STREAM = 6

MODULE_ENTRY_SIZE = 108
THREAD_ENTRY_SIZE = 48
MAX_FRAMES = 40

VSWHERE = Path(r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe")
SYMBOLIZER = r"VC\Tools\MSVC\**\bin\Hostx64\x64\llvm-symbolizer.exe"


class Module:
    def __init__(self, base: int, size: int, path: str) -> None:
        self.base, self.size, self.path = base, size, path

    def holds(self, address: int) -> bool:
        return self.base <= address < self.base + self.size


def streams(data: bytes) -> dict[int, int]:
    """Stream type -> file offset."""
    signature, _, count, directory = struct.unpack_from("<4sIII", data, 0)
    if signature != b"MDMP":
        raise SystemExit(f"not a minidump: {signature!r}")
    entries = (struct.unpack_from("<III", data, directory + i * 12) for i in range(count))
    return {kind: offset for kind, _, offset in entries}


def modules(data: bytes, offset: int) -> list[Module]:
    found = []
    for index in range(struct.unpack_from("<I", data, offset)[0]):
        entry = offset + 4 + index * MODULE_ENTRY_SIZE
        base, size, _, _, name_offset = struct.unpack_from("<QIIII", data, entry)
        length = struct.unpack_from("<I", data, name_offset)[0]
        name = data[name_offset + 4 : name_offset + 4 + length].decode("utf-16-le")
        found.append(Module(base, size, name))
    return found


def stack_scan(data: bytes, offset: int, thread_id: int, module: Module) -> list[int]:
    """RVAs into `module` on the thread's stack, innermost first, repeats collapsed."""
    for index in range(struct.unpack_from("<I", data, offset)[0]):
        entry = struct.unpack_from("<IIIIQQII", data, offset + 4 + index * THREAD_ENTRY_SIZE)
        if entry[0] != thread_id:
            continue
        _, _, _, _, _, _, stack_size, stack_offset = entry
        rvas: list[int] = []
        for slot in range(0, stack_size - 7, 8):
            rva = struct.unpack_from("<Q", data, stack_offset + slot)[0] - module.base
            if 0 < rva < module.size and (not rvas or rvas[-1] != rva):
                rvas.append(rva)
        return rvas[:MAX_FRAMES]
    return []


def image_base(path: Path) -> int:
    header = path.read_bytes()[:0x400]
    pe = struct.unpack_from("<I", header, 0x3C)[0]
    return struct.unpack_from("<Q", header, pe + 24 + 24)[0]


def symbolizer() -> str | None:
    if not VSWHERE.is_file():
        return None
    found = subprocess.run(
        [VSWHERE, "-latest", "-find", SYMBOLIZER], capture_output=True, text=True
    ).stdout.splitlines()
    return found[-1] if found else None


def symbolize(path: Path, rvas: list[int]) -> list[str]:
    """`function at file:line` per RVA; inlined frames are joined innermost first."""
    tool = symbolizer()
    if not rvas or tool is None or not path.is_file():
        return ["?"] * len(rvas)
    base = image_base(path)
    output = subprocess.run(
        [tool, f"--obj={path}", *(hex(base + rva) for rva in rvas)], capture_output=True, text=True
    ).stdout
    blocks = output.strip().split("\n\n")
    lines = []
    for block in blocks:
        rows = block.splitlines()
        frames = [f"{rows[i]} at {rows[i + 1]}" for i in range(0, len(rows) - 1, 2)]
        lines.append(" <- ".join(frames) if frames and "??" not in block else "?")
    return lines + ["?"] * (len(rvas) - len(lines))


def main() -> None:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument("dump", type=Path)
    parser.add_argument("--module", help="Substring of the module to list frames for")
    parser.add_argument("--dll", type=Path, help="Resolve that module against this file instead")
    args = parser.parse_args()

    data = args.dump.read_bytes()
    offsets = streams(data)
    loaded = modules(data, offsets[MODULE_LIST_STREAM])
    if EXCEPTION_STREAM not in offsets:
        raise SystemExit("no exception record: a hand-written dump, not a crash")

    record = offsets[EXCEPTION_STREAM]
    thread_id = struct.unpack_from("<I", data, record)[0]
    code, _, _, address, parameter_count = struct.unpack_from("<IIQQI", data, record + 8)
    parameters = struct.unpack_from("<15Q", data, record + 40)[:parameter_count]
    faulting = next((module for module in loaded if module.holds(address)), None)
    target = faulting
    if args.module:
        target = next((m for m in loaded if args.module.lower() in m.path.lower()), None)
        if target is None:
            raise SystemExit(f"no module matching {args.module!r}")

    def file_of(module: Module) -> Path:
        return args.dll if args.dll and module is target else Path(module.path)

    print(f"code    : 0x{code:08x} on thread {thread_id}")
    if code == 0xC0000005 and parameter_count >= 2:
        access = {0: "read", 1: "write", 8: "execute"}.get(parameters[0], str(parameters[0]))
        print(f"access  : {access} of 0x{parameters[1]:x}")
    if faulting is None:
        print(f"fault   : 0x{address:x} outside every module")
    else:
        rva = address - faulting.base
        print(f"fault   : {faulting.path}+0x{rva:x}")
        print(f"          {symbolize(file_of(faulting), [rva])[0]}")
    if target is None:
        return

    path = file_of(target)
    if path.is_file() and path.stat().st_mtime > args.dump.stat().st_mtime:
        print(f"WARNING: {path} changed after the crash; its lines are for another build")
    rvas = stack_scan(data, offsets[THREAD_LIST_STREAM], thread_id, target)
    print(f"\n{len(rvas)} frames in {Path(target.path).name}, innermost first:")
    for rva, line in zip(rvas, symbolize(path, rvas), strict=True):
        print(f"  0x{rva:<8x} {line}")


if __name__ == "__main__":
    main()
