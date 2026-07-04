#!/usr/bin/env python3
"""Format plugin + vendored cs2-kit C++ sources. Usage: format.py [--check]"""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

# Shared helpers live in the cs2-kit submodule (single source of truth), mirroring
# scripts/build.py.
sys.path.insert(0, str(ROOT / "vendor/cs2-kit/scripts"))
import buildtools  # noqa: E402

DIRS = [
    "plugins",
    "vendor/cs2-kit/src",
    "vendor/cs2-kit/include",
    "vendor/cs2-kit/tests",
]


def main() -> None:
    buildtools.format_sources(ROOT, DIRS, check="--check" in sys.argv[1:])


if __name__ == "__main__":
    main()
