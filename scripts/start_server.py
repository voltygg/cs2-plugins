#!/usr/bin/env python3
"""Start the CS2 dedicated server (optionally updating it via SteamCMD first)."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


def update_server(steamcmd: Path, server_path: str) -> None:
    """Update/validate the server via SteamCMD; warn but continue on failure."""
    if not steamcmd.is_file():
        print(f"WARNING: SteamCMD not found at {steamcmd}. Skipping update.")
        print("Install SteamCMD or use --steamcmd-path to specify its location.\n")
        return

    print("=== Updating CS2 Dedicated Server ===")
    result = subprocess.run(
        [
            str(steamcmd),
            "+force_install_dir",
            server_path,
            "+login",
            "anonymous",
            "+app_update",
            "730",
            "validate",
            "+quit",
        ]
    )
    if result.returncode != 0:
        print(
            f"WARNING: SteamCMD update failed (exit code {result.returncode}). "
            "Continuing with existing files..."
        )
    else:
        print("Server update complete.")
    print()


def main(argv: list[str] | None = None) -> None:
    """Parse arguments and launch cs2.exe."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server-path", default="C:/cs2-server")
    parser.add_argument("--steamcmd-path", default="C:/Program Files/steamcmd/steamcmd.exe")
    parser.add_argument("--map", default="de_dust2")
    parser.add_argument("--gslt-token", default="")
    parser.add_argument("--max-players", type=int, default=64)
    parser.add_argument("--port", type=int, default=27015)
    parser.add_argument("--rcon-password", default="")
    parser.add_argument("--check-update", action="store_true")
    args = parser.parse_args(argv)

    if args.check_update:
        update_server(Path(args.steamcmd_path), args.server_path)

    win64 = Path(args.server_path) / "game" / "bin" / "win64"
    cs2_exe = win64 / "cs2.exe"
    if not cs2_exe.is_file():
        raise SystemExit(
            f"ERROR: CS2 executable not found: {cs2_exe}\n"
            "Please specify a valid server path with --server-path"
        )

    print("=== Starting CS2 Dedicated Server ===\n")
    print(f"Server Path: {args.server_path}")
    print(f"Map: {args.map}")
    print(f"Max Players: {args.max_players}")
    print(f"Port: {args.port}")
    print(f"GSLT: {'Enabled (public server)' if args.gslt_token else 'Not set (LAN mode)'}\n")

    cmd = [
        str(cs2_exe),
        "-dedicated",
        "-console",
        "-usercon",
        "+map",
        args.map,
        "-maxplayers",
        str(args.max_players),
        "-port",
        str(args.port),
        "+game_mode",
        "0",
    ]
    if args.gslt_token:
        cmd += ["+sv_setsteamaccount", args.gslt_token]
    if args.rcon_password:
        cmd += ["+rcon_password", args.rcon_password]

    print("Starting server...\nPress Ctrl+C to stop the server\n")
    subprocess.run(cmd, cwd=win64)


if __name__ == "__main__":
    main()
