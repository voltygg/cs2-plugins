#!/usr/bin/env python3
"""Unified Docker/VPS deployment CLI for CS2 plugin servers."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from common import DEPLOY, load_server_env, repo_path


def cmd_matrix(args: argparse.Namespace) -> None:
    """Print the deploy matrix consumed by GitHub Actions."""
    import inventory

    servers = inventory.active_servers(inventory.load())
    if args.server:
        servers = [server for server in servers if server.get("id") == args.server]
        if not servers:
            raise SystemExit(f"ERROR: server '{args.server}' not found in active inventory servers")
    print(json.dumps(servers))


def cmd_plugins(args: argparse.Namespace) -> None:
    """Print declared plugin names."""
    import inventory

    plugins = inventory.used_plugins(inventory.load())
    print("\n".join(plugins) if args.format == "plain" else json.dumps(plugins))


def cmd_runtime_image(_args: argparse.Namespace) -> None:
    """Print the configured runtime image ref."""
    import inventory

    print(inventory.runtime_image(inventory.load()))


def cmd_package(args: argparse.Namespace) -> None:
    """Run the package subcommand."""
    import bundle

    bundle.package_plugin(args.plugin, args.platform, args.out)


def cmd_render(args: argparse.Namespace) -> None:
    """Run the render subcommand."""
    import render as renderer

    if not args.no_env:
        load_server_env(args.server, required=False)
    out_dir = Path(args.out_dir) if args.out_dir else DEPLOY / ".render" / args.server
    renderer.render(args.server, repo_path(args.package_dir), out_dir, args.runtime_image)


def cmd_deploy(args: argparse.Namespace) -> None:
    """Run the deploy subcommand."""
    import remote

    remote.deploy_server(
        args.server,
        args.package_dir,
        args.runtime_image,
        dry_run=args.dry_run,
    )


def cmd_cleanup(args: argparse.Namespace) -> None:
    """Run the cleanup subcommand."""
    import remote

    remote.cleanup_server(args.server, yes=args.yes, dry_run=args.dry_run)


def cmd_ensure_dbs(args: argparse.Namespace) -> None:
    """Run the ensure-dbs subcommand."""
    import database

    database.ensure_databases(args.server, args.admin_user, dry_run=args.dry_run)


def cmd_tunnel_db(args: argparse.Namespace) -> None:
    """Run the tunnel-db subcommand."""
    import remote

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

    matrix = sub.add_parser("matrix", help="emit the active deploy server matrix as JSON")
    matrix.add_argument("--server", help="filter to one server id and fail if missing")
    matrix.set_defaults(func=cmd_matrix)

    plugins = sub.add_parser("plugins", help="emit declared deploy plugins")
    plugins.add_argument("--format", choices=("json", "plain"), default="json")
    plugins.set_defaults(func=cmd_plugins)

    runtime = sub.add_parser("runtime-image", help="emit the configured server runtime image")
    runtime.set_defaults(func=cmd_runtime_image)

    package = sub.add_parser("package", help="package one built plugin for Docker deploy")
    package.add_argument("plugin")
    package.add_argument("platform", choices=("linux", "windows"), nargs="?", default="linux")
    package.add_argument("--out")
    package.set_defaults(func=cmd_package)

    render = sub.add_parser("render", help="render compose artifacts for one server")
    render.add_argument("--server", required=True)
    render.add_argument("--package-dir", default="package")
    render.add_argument("--out-dir")
    render.add_argument("--runtime-image")
    render.add_argument("--no-env", action="store_true", help="do not load server .env first")
    render.set_defaults(func=cmd_render)

    deploy = sub.add_parser("deploy", help="render and deploy one server over SSH")
    deploy.add_argument("--server", required=True)
    deploy.add_argument("--package-dir", default="package")
    deploy.add_argument("--runtime-image")
    deploy.add_argument("--dry-run", action="store_true")
    deploy.set_defaults(func=cmd_deploy)

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
