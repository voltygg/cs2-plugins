"""Unified deployment CLI for CS2 plugin servers (Docker/VPS and local).

Run as a module from the repo root: python -m deploy.tools.cli <subcommand>
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from .common import DEPLOY, die, load_server_env, materialize_server_env, repo_path


def cmd_matrix(args: argparse.Namespace) -> None:
    """Print the deploy matrix: full JSON for the GHA matrix, ids only for shell loops."""
    from . import inventory

    servers = inventory.active_servers(inventory.load())
    if args.server:
        servers = [server for server in servers if server.get("id") == args.server]
        if not servers:
            raise SystemExit(f"ERROR: server '{args.server}' not found in active inventory servers")
    if args.format == "plain":
        print("\n".join(server["id"] for server in servers))
    else:
        print(json.dumps(servers))


def cmd_plugins(args: argparse.Namespace) -> None:
    """Print declared plugin names."""
    from . import inventory

    plugins = inventory.used_plugins(inventory.load())
    print("\n".join(plugins) if args.format == "plain" else json.dumps(plugins))


def cmd_runtime_image(_args: argparse.Namespace) -> None:
    """Print the configured runtime image ref."""
    from . import inventory

    print(inventory.runtime_image(inventory.load()))


def cmd_package(args: argparse.Namespace) -> None:
    from . import bundle, inventory

    if args.all:
        plugins = inventory.used_plugins(inventory.load())
    elif args.plugin:
        plugins = [args.plugin]
    else:
        die("package needs a plugin name or --all")

    for plugin in plugins:
        bundle.package_plugin(plugin, args.platform, args.out)


def cmd_render(args: argparse.Namespace) -> None:
    from . import render as renderer

    if not args.no_env:
        load_server_env(args.server, required=False)
    out_dir = Path(args.out_dir) if args.out_dir else DEPLOY / ".render" / args.server
    renderer.render(args.server, repo_path(args.package_dir), out_dir, args.runtime_image)


def cmd_deploy(args: argparse.Namespace) -> None:
    from . import inventory, remote

    data = inventory.load()
    if args.all:
        servers = [server["id"] for server in inventory.active_servers(data)]
    elif args.server:
        servers = [args.server]
    else:
        die("deploy needs --server or --all")

    runtime_image = args.runtime_image or inventory.runtime_image(data)

    for server_id in servers:
        materialize_server_env(server_id)
        remote.deploy_server(
            server_id,
            args.package_dir,
            runtime_image,
            dry_run=args.dry_run,
        )


def cmd_update(args: argparse.Namespace) -> None:
    from . import remote

    remote.update_server(args.server, dry_run=args.dry_run)


def cmd_cleanup(args: argparse.Namespace) -> None:
    from . import remote

    remote.cleanup_server(args.server, yes=args.yes, dry_run=args.dry_run)


def cmd_ensure_dbs(args: argparse.Namespace) -> None:
    from . import database

    database.ensure_databases(args.server, args.admin_user, dry_run=args.dry_run)


def cmd_rcon(args: argparse.Namespace) -> None:
    from . import rcon

    rcon.run_commands(args.commands, server_id=args.server, instance_name=args.instance)


def cmd_local(args: argparse.Namespace) -> None:
    from . import local

    local.deploy_local(args.server_path, args.plugin_name)


def cmd_start(args: argparse.Namespace) -> None:
    from . import local

    local.start_server(
        args.server_path,
        args.steamcmd_path,
        args.map,
        args.gslt_token,
        args.max_players,
        args.port,
        args.rcon_password,
        check_update=args.check_update,
    )


def cmd_tunnel_db(args: argparse.Namespace) -> None:
    from . import remote

    remote.tunnel_db(
        server_id=args.server,
        host_arg=args.host,
        ssh_user_arg=args.ssh_user,
        ssh_port_arg=args.ssh_port,
        db_host_arg=args.db_host,
        db_port_arg=args.db_port,
        local_port_arg=args.local_port,
        identity_arg=args.identity,
    )


def build_parser() -> argparse.ArgumentParser:
    """Build the top-level deploy CLI parser."""
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    def command(name: str, help_text: str, handler) -> argparse.ArgumentParser:
        child = sub.add_parser(name, help=help_text)
        child.set_defaults(func=handler)
        return child

    matrix = command("matrix", "emit the active deploy server matrix", cmd_matrix)
    matrix.add_argument("--server", help="filter to one server id and fail if missing")
    matrix.add_argument("--format", choices=("json", "plain"), default="json")

    plugins = command("plugins", "emit declared deploy plugins", cmd_plugins)
    plugins.add_argument("--format", choices=("json", "plain"), default="json")

    command("runtime-image", "emit the configured server runtime image", cmd_runtime_image)

    package = command("package", "package built plugins for Docker deploy", cmd_package)
    package.add_argument("plugin", nargs="?")
    package.add_argument("platform", choices=("linux", "windows"), nargs="?", default="linux")
    package.add_argument("--all", action="store_true", help="every plugin the inventory declares")
    package.add_argument("--out")

    render = command("render", "render compose artifacts for one server", cmd_render)
    render.add_argument("--server", required=True)
    render.add_argument("--package-dir", default="package")
    render.add_argument("--out-dir")
    render.add_argument("--runtime-image")
    render.add_argument("--no-env", action="store_true", help="do not load server .env first")

    deploy = command("deploy", "render and deploy servers over SSH", cmd_deploy)
    deploy.add_argument("--server")
    deploy.add_argument("--all", action="store_true", help="every active inventory server")
    deploy.add_argument("--package-dir", default="package")
    deploy.add_argument("--runtime-image", help="default: the inventory's runtime image")
    deploy.add_argument("--dry-run", action="store_true")

    update = command("update", "restart instances to pull the latest CS2 build", cmd_update)
    update.add_argument("--server", required=True)
    update.add_argument("--dry-run", action="store_true")

    cleanup = command("cleanup", "remove one deployed CS2 server stack", cmd_cleanup)
    cleanup.add_argument("--server", required=True)
    cleanup.add_argument("--yes", action="store_true", help="confirm destructive cleanup")
    cleanup.add_argument("--dry-run", action="store_true", help="print cleanup actions only")

    dbs = command("ensure-dbs", "ensure the shared Postgres role/databases", cmd_ensure_dbs)
    dbs.add_argument("--admin-user", default="postgres")
    dbs.add_argument("--server")
    dbs.add_argument("--dry-run", action="store_true")

    local = command("local", "deploy built plugins into a local CS2 server tree", cmd_local)
    local.add_argument("--server-path", default="C:/cs2-server")
    local.add_argument("--plugin-name", default="")

    start = command("start", "start a local CS2 dedicated server", cmd_start)
    start.add_argument("--server-path", default="C:/cs2-server")
    start.add_argument("--steamcmd-path", default="C:/Program Files/steamcmd/steamcmd.exe")
    start.add_argument("--map", default="de_dust2")
    start.add_argument("--gslt-token", default="")
    start.add_argument("--max-players", type=int, default=64)
    start.add_argument("--port", type=int, default=27015)
    start.add_argument("--rcon-password", default="")
    start.add_argument("--check-update", action="store_true")

    rcon = command("rcon", "run console commands on a live instance over RCON", cmd_rcon)
    rcon.add_argument("commands", nargs="+", help="one or more console commands (quote each)")
    rcon.add_argument("--server", help="server id (optional when the inventory has one)")
    rcon.add_argument("--instance", help="instance name (optional when the server has one)")

    tunnel = command("tunnel-db", "open an SSH tunnel to a server Postgres", cmd_tunnel_db)
    tunnel.add_argument("--server")
    tunnel.add_argument("--host")
    tunnel.add_argument("--ssh-user")
    tunnel.add_argument("--ssh-port")
    tunnel.add_argument("--db-host")
    tunnel.add_argument("--db-port")
    tunnel.add_argument("--local-port")
    tunnel.add_argument("-i", "--identity")

    return parser


def main(argv: list[str] | None = None) -> None:
    """Parse arguments and dispatch to the selected subcommand."""
    args = build_parser().parse_args(argv)
    args.func(args)


if __name__ == "__main__":
    main()
