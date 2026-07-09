"""Build-output packaging for Docker deploy bundles."""

import os
import shutil
from pathlib import Path

from common import ROOT, die, repo_path, run


def package_plugin(plugin: str, platform: str, out: str | None) -> None:
    """Install one built plugin component into a deploy bundle directory."""
    build_preset = _build_preset(platform)
    plugin_dir = ROOT / "plugins" / plugin
    if not plugin_dir.is_dir():
        die(f"plugin '{plugin_dir.relative_to(ROOT)}' not found")

    build_dir = ROOT / "build" / build_preset
    if not build_dir.is_dir():
        die(
            f"no build at {build_dir.relative_to(ROOT)}; build first with "
            "`docker compose -f deploy/docker-compose.build.yml run --rm --build build`"
        )

    out_dir = repo_path(out or Path("package") / plugin).resolve()
    _check_output_dir(out_dir)

    print(f"=== Packaging {plugin} ({platform}) -> {out_dir} ===")
    if out_dir.exists():
        shutil.rmtree(out_dir)
    run(
        [
            "cmake",
            "--install",
            str(build_dir),
            "--component",
            plugin,
            "--prefix",
            str(out_dir),
        ],
        cwd=ROOT,
    )
    print(f"=== Bundle ready: {out_dir} ===")


def _build_preset(platform: str) -> str:
    if platform == "linux":
        return os.environ.get("CS2_BUILD_PRESET", "linux-steamrt-release")
    if platform == "windows":
        return os.environ.get("CS2_BUILD_PRESET", "windows-msvc-release")
    die(f"unknown platform '{platform}' (use linux|windows)")


def _check_output_dir(out_dir: Path) -> None:
    root = ROOT.resolve()
    if out_dir == root or not out_dir.is_relative_to(root):
        die(f"refusing unsafe output dir: {out_dir} (must resolve under {root})")
    if out_dir.exists() and not out_dir.is_dir():
        die(f"output path exists and is not a directory: {out_dir}")
