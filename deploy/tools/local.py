"""Deploy built plugin binaries, configs, and cs2-kit gamedata to a local CS2 server.

Run via poe (`uv run poe deploy`): it sets PYTHONPATH so the vendored kit's
buildtools resolves.
"""

import os
import shutil
from pathlib import Path

import buildtools

from .common import ROOT, die

BUILD_PRESET = os.environ.get("CS2_BUILD_PRESET", "windows-msvc-release")


def plugin_names(requested: str) -> list[str]:
    """Resolve the requested plugin, or every plugin under plugins/."""
    if requested:
        if not (ROOT / "plugins" / requested).is_dir():
            die(f"plugin 'plugins/{requested}' not found")
        return [requested]
    names = sorted(p.name for p in (ROOT / "plugins").iterdir() if p.is_dir())
    if not names:
        die("no plugins found under plugins/")
    return names


def deploy_plugin(name: str, csgo: Path, *, named: bool) -> None:
    """Stage one plugin via cmake --install and merge it into the server tree."""
    print(f"--- {name} ---")
    build_dir = ROOT / "build" / BUILD_PRESET
    if not build_dir.is_dir():
        if named:
            die(f"no build at {build_dir}\nBuild first: uv run poe build")
        print(f"  (skipped - no build at {build_dir})")
        return

    # Stage via the shared install() rules, then merge into the server tree.
    staging = build_dir / "_deploy-staging" / name
    shutil.rmtree(staging, ignore_errors=True)
    try:
        buildtools.run_tool(
            "cmake",
            "--install",
            str(build_dir),
            "--component",
            name,
            "--prefix",
            str(staging),
        )
    except Exception:
        if named:
            die(f"cmake --install failed for {name} (is it built?)")
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


def deploy_local(server_path: str, plugin_name: str) -> None:
    """Deploy the selected plugins into a local CS2 server tree."""
    csgo = Path(server_path) / "game" / "csgo"
    if not csgo.is_dir():
        die(
            f"CS2 server not found at {csgo}\n"
            "Please specify the correct server path with --server-path"
        )

    plugins = plugin_names(plugin_name)
    print("=== Metamod:Source Plugin Deployment ===\n")
    print(f"Server Path:   {server_path}")
    print(f"CSGO Path:     {csgo}")
    print(f"Build preset:  {BUILD_PRESET}\n")

    (csgo / "addons" / "metamod").mkdir(parents=True, exist_ok=True)
    for name in plugins:
        deploy_plugin(name, csgo, named=bool(plugin_name))

    print(f"\n=== Deployment Complete ===\nDeployed plugins: {' '.join(plugins)}")
    print("\nTo verify installation, run on server console: meta list")
