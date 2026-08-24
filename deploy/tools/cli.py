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

    if args.all:
        servers = [server["id"] for server in inventory.active_servers(inventory.load())]
    elif args.server:
        servers = [args.server]
    else:
        die("deploy needs --server or --all")

    # One image for the whole sweep, resolved once rather than per provider in YAML.
    runtime_image = args.runtime_image or inventory.runtime_image(inventory.load())

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

    matrix = sub.add_parser("matrix", help="emit the active deploy server matrix")
    matrix.add_argument("--server", help="filter to one server id and fail if missing")
    matrix.add_argument("--format", choices=("json", "plain"), default="json")
    matrix.set_defaults(func=cmd_matrix)

    plugins = sub.add_parser("plugins", help="emit declared deploy plugins")
    plugins.add_argument("--format", choices=("json", "plain"), default="json")
    plugins.set_defaults(func=cmd_plugins)

    runtime = sub.add_parser("runtime-image", help="emit the configured server runtime image")
    runtime.set_defaults(func=cmd_runtime_image)

    package = sub.add_parser("package", help="package built plugins for Docker deploy")
    package.add_argument("plugin", nargs="?")
    package.add_argument("platform", choices=("linux", "windows"), nargs="?", default="linux")
    package.add_argument("--all", action="store_true", help="every plugin the inventory declares")
    package.add_argument("--out")
    package.set_defaults(func=cmd_package)

    render = sub.add_parser("render", help="render compose artifacts for one server")
    render.add_argument("--server", required=True)
    render.add_argument("--package-dir", default="package")
    render.add_argument("--out-dir")
    render.add_argument("--runtime-image")
    render.add_argument("--no-env", action="store_true", help="do not load server .env first")
    render.set_defaults(func=cmd_render)

    deploy = sub.add_parser("deploy", help="render and deploy servers over SSH")
    deploy.add_argument("--server")
    deploy.add_argument("--all", action="store_true", help="every active inventory server")
    deploy.add_argument("--package-dir", default="package")
    deploy.add_argument("--runtime-image", help="default: the inventory's runtime image")
    deploy.add_argument("--dry-run", action="store_true")
    deploy.set_defaults(func=cmd_deploy)

    update = sub.add_parser("update", help="restart instances to pull the latest CS2 build")
    update.add_argument("--server", required=True)
    update.add_argument("--dry-run", action="store_true")
    update.set_defaults(func=cmd_update)

    cleanup = sub.add_parser("cleanup", help="remove one deployed CS2 server stack")
    cleanup.add_argument("--server", required=True)
    cleanup.add_argument("--yes", action="store_true", help="confirm destructive cleanup")
    cleanup.add_argument("--dry-run", action="store_true", help="print cleanup actions only")
    cleanup.set_defaults(func=cmd_cleanup)

    dbs = sub.add_parser("ensure-dbs", help="ensure the shared Postgres role/databases")
    dbs.add_argument("--admin-user", default="postgres")
    dbs.add_argument("--server")
    dbs.add_argument("--dry-run", action="store_true")
    dbs.set_defaults(func=cmd_ensure_dbs)

    local = sub.add_parser("local", help="deploy built plugins into a local CS2 server tree")
    local.add_argument("--server-path", default="C:/cs2-server")
    local.add_argument("--plugin-name", default="")
    local.set_defaults(func=cmd_local)

    rcon = sub.add_parser("rcon", help="run console commands on a live instance over RCON")
    rcon.add_argument("commands", nargs="+", help="one or more console commands (quote each)")
    rcon.add_argument("--server", help="server id (optional when the inventory has one)")
    rcon.add_argument("--instance", help="instance name (optional when the server has one)")
    rcon.set_defaults(func=cmd_rcon)

    tunnel = sub.add_parser("tunnel-db", help="open an SSH tunnel to a server Postgres")
    tunnel.add_argument("--server")
    tunnel.add_argument("--host")
    tunnel.add_argument("--ssh-user")
    tunnel.add_argument("--ssh-port")
    tunnel.add_argument("--db-host")
    tunnel.add_argument("--db-port")
    tunnel.add_argument("--local-port")
    tunnel.add_argument("-i", "--identity")
    tunnel.set_defaults(func=cmd_tunnel_db)

    return parser


def main(argv: list[str] | None = None) -> None:
    """Parse arguments and dispatch to the selected subcommand."""
    args = build_parser().parse_args(argv)
    args.func(args)


if __name__ == "__main__":
    main()
