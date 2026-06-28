#!/usr/bin/env python3
"""Build the plugins with Conan + CMake. Usage: build.py [preset]"""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

# Shared helpers live in the cs2-kit submodule (single source of truth), mirroring
# how scripts/lib/common.sh used to source them.
sys.path.insert(0, str(ROOT / "vendor/cs2-kit/scripts"))
import buildtools  # noqa: E402


def main(argv: list[str] | None = None) -> None:
    """Build the requested preset (default: release for this OS)."""
    args = sys.argv[1:] if argv is None else argv
    preset = args[0] if args else buildtools.default_preset()
    buildtools.build(ROOT, preset, workflow=True)


if __name__ == "__main__":
    main()
