"""Unified deployment CLI for CS2 plugin servers (Docker/VPS and local).

Run as a module from the repo root: python -m deploy.tools.cli <subcommand>
"""

from __future__ import annotations

import json
from enum import StrEnum
from pathlib import Path
from typing import Annotated

import typer

from .common import (
    DEPLOY,
    die,
    load_local_env,
    load_server_env,
    materialize_server_env,
    repo_path,
)

app = typer.Typer(help=__doc__, no_args_is_help=True)

ServerPath = Annotated[str, typer.Option("--server-path", envvar="CS2_SERVER_PATH")]
SteamcmdPath = Annotated[str, typer.Option("--steamcmd-path", envvar="STEAMCMD_PATH")]
MapName = Annotated[str, typer.Option("--map", envvar="CS2_MAP")]
GsltToken = Annotated[str, typer.Option("--gslt-token", envvar="GSLT_TOKEN")]
MaxPlayers = Annotated[int, typer.Option("--max-players", envvar="CS2_MAX_PLAYERS")]
ServerPort = Annotated[int, typer.Option("--port", envvar="CS2_PORT")]
RconPassword = Annotated[str, typer.Option("--rcon-password", envvar="RCON_PASSWORD")]


class OutputFormat(StrEnum):
    JSON = "json"
    PLAIN = "plain"


class Platform(StrEnum):
    LINUX = "linux"
    WINDOWS = "windows"


@app.callback()
def configure() -> None:
    """Load local defaults before Typer parses the selected command."""
    load_local_env()


@app.command()
def matrix(
    server: Annotated[
        str | None,
        typer.Option("--server", help="Filter to one server id and fail if missing"),
    ] = None,
    format_: Annotated[
        OutputFormat,
        typer.Option("--format", help="Output format"),
    ] = OutputFormat.JSON,
) -> None:
    """Emit the active deploy server matrix."""
    from . import inventory

    servers = inventory.active_servers(inventory.load())
    if server:
        servers = [item for item in servers if item.get("id") == server]
        if not servers:
            raise SystemExit(f"ERROR: server '{server}' not found in active inventory servers")
    if format_ is OutputFormat.PLAIN:
        print("\n".join(item["id"] for item in servers))
    else:
        print(json.dumps(servers))


@app.command()
def plugins(
    format_: Annotated[
        OutputFormat,
        typer.Option("--format", help="Output format"),
    ] = OutputFormat.JSON,
) -> None:
    """Emit declared deploy plugins."""
    from . import inventory

    names = inventory.used_plugins(inventory.load())
    print("\n".join(names) if format_ is OutputFormat.PLAIN else json.dumps(names))


@app.command("runtime-image")
def runtime_image() -> None:
    """Emit the configured server runtime image."""
    from . import inventory

    print(inventory.runtime_image(inventory.load()))


@app.command()
def package(
    plugin: Annotated[str | None, typer.Argument()] = None,
    platform: Annotated[Platform, typer.Argument()] = Platform.LINUX,
    all_: Annotated[
        bool,
        typer.Option("--all", help="Every plugin the inventory declares"),
    ] = False,
    out: Annotated[str | None, typer.Option("--out")] = None,
) -> None:
    """Package built plugins for Docker deployment."""
    from . import bundle, inventory

    if all_:
        selected = inventory.used_plugins(inventory.load())
    elif plugin:
        selected = [plugin]
    else:
        die("package needs a plugin name or --all")

    for name in selected:
        bundle.package_plugin(name, platform.value, out)


@app.command()
def render(
    server: Annotated[str, typer.Option("--server")],
    package_dir: Annotated[str, typer.Option("--package-dir")] = "package",
    out_dir: Annotated[str | None, typer.Option("--out-dir")] = None,
    runtime_image: Annotated[str | None, typer.Option("--runtime-image")] = None,
    no_env: Annotated[
        bool,
        typer.Option("--no-env", help="Do not load server .env first"),
    ] = False,
) -> None:
    """Render compose artifacts for one server."""
    from . import render as renderer

    if not no_env:
        load_server_env(server, required=False)
    output = Path(out_dir) if out_dir else DEPLOY / ".render" / server
    renderer.render(server, repo_path(package_dir), output, runtime_image)


@app.command()
def deploy(
    server: Annotated[str | None, typer.Option("--server")] = None,
    all_: Annotated[
        bool,
        typer.Option("--all", help="Every active inventory server"),
    ] = False,
    package_dir: Annotated[str, typer.Option("--package-dir")] = "package",
    runtime_image: Annotated[
        str | None,
        typer.Option("--runtime-image", help="Default: the inventory's runtime image"),
    ] = None,
    dry_run: Annotated[bool, typer.Option("--dry-run")] = False,
) -> None:
    """Render and deploy servers over SSH."""
    from . import inventory, remote

    data = inventory.load()
    if all_:
        servers = [item["id"] for item in inventory.active_servers(data)]
    elif server:
        servers = [server]
    else:
        die("deploy needs --server or --all")

    image = runtime_image or inventory.runtime_image(data)
    for server_id in servers:
        materialize_server_env(server_id)
        remote.deploy_server(server_id, package_dir, image, dry_run=dry_run)


@app.command()
def update(
    server: Annotated[str, typer.Option("--server")],
    dry_run: Annotated[bool, typer.Option("--dry-run")] = False,
) -> None:
    """Restart instances to pull the latest CS2 build."""
    from . import remote

    remote.update_server(server, dry_run=dry_run)


@app.command()
def cleanup(
    server: Annotated[str, typer.Option("--server")],
    yes: Annotated[bool, typer.Option("--yes", help="Confirm destructive cleanup")] = False,
    dry_run: Annotated[
        bool,
        typer.Option("--dry-run", help="Print cleanup actions only"),
    ] = False,
) -> None:
    """Remove one deployed CS2 server stack."""
    from . import remote

    remote.cleanup_server(server, yes=yes, dry_run=dry_run)


@app.command("ensure-dbs")
def ensure_dbs(
    admin_user: Annotated[str, typer.Option("--admin-user")] = "postgres",
    server: Annotated[str | None, typer.Option("--server")] = None,
    dry_run: Annotated[bool, typer.Option("--dry-run")] = False,
) -> None:
    """Ensure the shared Postgres role and databases."""
    from . import database

    database.ensure_databases(server, admin_user, dry_run=dry_run)


@app.command("local")
def deploy_local(
    server_path: ServerPath = "C:/cs2-server",
    plugin_name: Annotated[str, typer.Option("--plugin-name")] = "",
) -> None:
    """Deploy built plugins into a local CS2 server tree."""
    from . import local

    local.deploy_local(server_path, plugin_name)


@app.command()
def start(
    server_path: ServerPath = "C:/cs2-server",
    steamcmd_path: SteamcmdPath = "C:/Program Files/steamcmd/steamcmd.exe",
    map_: MapName = "de_dust2",
    gslt_token: GsltToken = "",
    max_players: MaxPlayers = 16,
    port: ServerPort = 27015,
    rcon_password: RconPassword = "",
    check_update: Annotated[bool, typer.Option("--check-update")] = False,
) -> None:
    """Start a local CS2 dedicated server."""
    from . import local

    local.start_server(
        server_path,
        steamcmd_path,
        map_,
        gslt_token,
        max_players,
        port,
        rcon_password,
        check_update=check_update,
    )


@app.command()
def dev(
    plugin: Annotated[str, typer.Argument()],
    server_path: ServerPath = "C:/cs2-server",
    no_test: Annotated[
        bool,
        typer.Option("--no-test", help="Skip CTest for a faster iteration"),
    ] = False,
    start_server: Annotated[
        bool,
        typer.Option("--start", help="Launch the server after installation"),
    ] = False,
    steamcmd_path: SteamcmdPath = "C:/Program Files/steamcmd/steamcmd.exe",
    map_: MapName = "de_dust2",
    gslt_token: GsltToken = "",
    max_players: MaxPlayers = 16,
    port: ServerPort = 27015,
    rcon_password: RconPassword = "",
    check_update: Annotated[bool, typer.Option("--check-update")] = False,
) -> None:
    """Build, test, and install one plugin locally."""
    from . import local

    local.develop_plugin(
        plugin,
        server_path,
        run_tests=not no_test,
        launch=start_server,
        steamcmd_path=steamcmd_path,
        map_name=map_,
        gslt_token=gslt_token,
        max_players=max_players,
        port=port,
        rcon_password=rcon_password,
        check_update=check_update,
    )


@app.command()
def rcon(
    commands: Annotated[
        list[str],
        typer.Argument(help="One or more console commands (quote each)"),
    ],
    server: Annotated[
        str | None,
        typer.Option("--server", help="Server id (optional when inventory has one)"),
    ] = None,
    instance: Annotated[
        str | None,
        typer.Option("--instance", help="Instance name (optional when server has one)"),
    ] = None,
) -> None:
    """Run console commands on a live instance over RCON."""
    from . import rcon as rcon_client

    rcon_client.run_commands(commands, server_id=server, instance_name=instance)


@app.command("tunnel-db")
def tunnel_db(
    server: Annotated[str | None, typer.Option("--server")] = None,
    host: Annotated[str | None, typer.Option("--host")] = None,
    ssh_user: Annotated[str | None, typer.Option("--ssh-user")] = None,
    ssh_port: Annotated[str | None, typer.Option("--ssh-port")] = None,
    db_host: Annotated[str | None, typer.Option("--db-host")] = None,
    db_port: Annotated[str | None, typer.Option("--db-port")] = None,
    local_port: Annotated[str | None, typer.Option("--local-port")] = None,
    identity: Annotated[str | None, typer.Option("--identity", "-i")] = None,
) -> None:
    """Open an SSH tunnel to a server Postgres database."""
    from . import remote

    remote.tunnel_db(
        server_id=server,
        host_arg=host,
        ssh_user_arg=ssh_user,
        ssh_port_arg=ssh_port,
        db_host_arg=db_host,
        db_port_arg=db_port,
        local_port_arg=local_port,
        identity_arg=identity,
    )


def main(argv: list[str] | None = None) -> None:
    app(args=argv)


if __name__ == "__main__":
    main()
