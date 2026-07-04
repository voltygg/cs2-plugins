#!/usr/bin/env python3
"""One-command setup: fetch submodules, check build tools, then build."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

sys.path.insert(0, str(ROOT / "vendor/cs2-kit/scripts"))
import buildtools  # noqa: E402


def main() -> None:
    """Fetch submodules (cs2-kit + nested SDKs), verify tools, build."""
    print("==> [1/3] Fetching submodules (cs2-kit + nested SDKs)")
    subprocess.run(
        ["git", "submodule", "update", "--init", "--recursive", "--depth", "1"],
        cwd=ROOT,
        check=True,
    )

    print("==> [2/3] Checking CMake + Conan build tools")
    buildtools.require_build_tools()

    print("==> [3/3] Building with Conan + CMake")
    buildtools.build(ROOT, buildtools.default_preset(), workflow=True)

    print(
        "\n============================================================\n"
        "  Bootstrap complete - the plugin built successfully.\n"
        "  Output: build/<preset>/plugins/\n"
        "============================================================"
    )


if __name__ == "__main__":
    main()
