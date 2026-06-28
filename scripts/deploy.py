#!/usr/bin/env python3
"""Deploy built plugin binaries, configs, and cs2-kit gamedata to a local CS2 server."""

from __future__ import annotations

import argparse
import os
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

sys.path.insert(0, str(ROOT / "vendor/cs2-kit/scripts"))
import buildtools  # noqa: E402

BUILD_PRESET = os.environ.get("CS2_BUILD_PRESET", "windows-msvc-release")


def plugin_names(requested: str) -> list[str]:
    """Resolve the requested plugin, or every plugin under plugins/."""
    if requested:
        if not (ROOT / "plugins" / requested).is_dir():
            buildtools.die(f"plugin 'plugins/{requested}' not found")
        return [requested]
    names = sorted(p.name for p in (ROOT / "plugins").iterdir() if p.is_dir())
    if not names:
        buildtools.die("no plugins found under plugins/")
    return names


def deploy_plugin(name: str, csgo: Path, *, named: bool) -> None:
    """Stage one plugin via cmake --install and merge it into the server tree."""
    print(f"--- {name} ---")
    build_dir = ROOT / "build" / BUILD_PRESET
    if not build_dir.is_dir():
        if named:
            buildtools.die(f"no build at {build_dir}\nBuild first: scripts/build.py")
        print(f"  (skipped - no build at {build_dir})")
        return

    # Stage via the shared install() rules, then merge into the server tree.
    staging = build_dir / "_deploy-staging" / name
    shutil.rmtree(staging, ignore_errors=True)
    try:
        buildtools.run_tool(
            "cmake", "--install", str(build_dir),
            "--component", name, "--prefix", str(staging),
        )
    except Exception:
        if named:
            buildtools.die(f"cmake --install failed for {name} (is it built?)")
        print(f"  (skipped - cmake --install produced nothing for {name})")
        return

    shutil.copytree(staging / "addons", csgo / "addons", dirs_exist_ok=True)
    print("  -> addons/ (binary, vdf, configs, cs2-kit gamedata)")

    # settings.jsonc is seeded on first deploy and never clobbered.
    settings_src = ROOT / "plugins" / name / "configs" / "settings.jsonc"
    settings_dst = csgo / "addons" / name / "configs" / "settings.jsonc"
    if settings_src.is_file():
        if settings_dst.is_file():
            print("  -> configs/settings.jsonc (skipped - already exists)")
        else:
            settings_dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(settings_src, settings_dst)
            print("  -> configs/settings.jsonc (seeded)")


def main(argv: list[str] | None = None) -> None:
    """Parse arguments and deploy the selected plugins."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server-path", default="C:/cs2-server")
    parser.add_argument("--plugin-name", default="")
    parser.add_argument("--architecture", default="x86_64")
    args = parser.parse_args(argv)

    if args.architecture != "x86_64":
        buildtools.die("only x86_64 is supported by the CMake build.")

    csgo = Path(args.server_path) / "game" / "csgo"
    if not csgo.is_dir():
        buildtools.die(
            f"CS2 server not found at {csgo}\n"
            "Please specify the correct server path with --server-path"
        )

    plugins = plugin_names(args.plugin_name)
    print("=== Metamod:Source Plugin Deployment ===\n")
    print(f"Server Path:   {args.server_path}")
    print(f"CSGO Path:     {csgo}")
    print(f"Build preset:  {BUILD_PRESET}\n")

    (csgo / "addons" / "metamod").mkdir(parents=True, exist_ok=True)
    for name in plugins:
        deploy_plugin(name, csgo, named=bool(args.plugin_name))

    print(f"\n=== Deployment Complete ===\nDeployed plugins: {' '.join(plugins)}")
    print("\nTo verify installation, run on server console: meta list")


if __name__ == "__main__":
    main()
