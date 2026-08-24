"""Local CS2 server deployment and launch helpers."""

import os
import shutil
import subprocess
from pathlib import Path

from voltmod import buildtools

from .common import ROOT, die

BUILD_PRESET = os.environ.get("CS2_BUILD_PRESET", "windows-msvc-release")


def update_server(steamcmd: Path, server_path: Path) -> None:
    """Update the server when SteamCMD is available."""
    if not steamcmd.is_file():
        print(f"WARNING: SteamCMD not found at {steamcmd}; skipping update.")
        return

    result = subprocess.run(
        [
            str(steamcmd),
            "+force_install_dir",
            str(server_path),
            "+login",
            "anonymous",
            "+app_update",
            "730",
            "validate",
            "+quit",
        ]
    )
    if result.returncode:
        print(f"WARNING: SteamCMD update failed ({result.returncode}); using existing files.")


def start_server(
    server_path: str,
    steamcmd_path: str,
    map_name: str,
    gslt_token: str,
    max_players: int,
    port: int,
    rcon_password: str,
    *,
    check_update: bool,
) -> None:
    """Optionally update, then run the local CS2 dedicated server."""
    root = Path(server_path)
    if check_update:
        update_server(Path(steamcmd_path), root)

    win64 = root / "game" / "bin" / "win64"
    executable = win64 / "cs2.exe"
    if not executable.is_file():
        die(f"CS2 executable not found: {executable}")

    command = [
        str(executable),
        "-dedicated",
        "-console",
        "-usercon",
        "+map",
        map_name,
        "-maxplayers",
        str(max_players),
        "-port",
        str(port),
        "+game_mode",
        "0",
    ]
    if gslt_token:
        command += ["+sv_setsteamaccount", gslt_token]
    if rcon_password:
        command += ["+rcon_password", rcon_password]

    mode = "public" if gslt_token else "LAN"
    print(f"=== Starting CS2: {map_name}, {max_players} players, port {port}, {mode} ===")
    subprocess.run(command, cwd=win64)


def plugin_names(requested: str) -> list[str]:
    """Resolve the requested plugin, or every plugin under plugins/."""
    if requested:
        if not (ROOT / "plugins" / requested).is_dir():
            die(f"plugin 'plugins/{requested}' not found")
        return [requested]
    names = sorted(
        path.name
        for path in (ROOT / "plugins").iterdir()
        if path.is_dir() and (path / "src").is_dir()
    )
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
    except subprocess.CalledProcessError:
        if named:
            die(f"cmake --install failed for {name} (is it built?)")
        print(f"  (skipped - cmake --install produced nothing for {name})")
        return

    shutil.copytree(staging / "addons", csgo / "addons", dirs_exist_ok=True)
    print("  -> addons/ (binary, vdf, configs, voltmod gamedata)")

    # Preserve operator-edited settings after the first deploy.
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
