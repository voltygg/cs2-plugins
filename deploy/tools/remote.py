"""Remote SSH operations for Docker deploy hosts."""

import os
import shlex
import subprocess
import sys
from pathlib import Path
from typing import Any

import inventory
import render
from common import DEPLOY, command_line, die, load_server_env, repo_path, run


def deploy_server(
    server_id: str,
    package_dir: str,
    runtime_image: str | None,
    *,
    dry_run: bool,
) -> None:
    server = inventory.find_server(inventory.load(), server_id)
    load_server_env(server_id, required=True)

    render_dir = DEPLOY / ".render" / server_id
    render.render(server_id, repo_path(package_dir), render_dir, runtime_image)

    remote_root = str(server["deploy_root"])
    remote_root_q = shlex.quote(remote_root)
    target = ssh_target(server)

    print(f"=== Deploying Dockerized CS2 to {server_id} ({target}:{remote_root}) ===")
    print(f"    plugins:   {' '.join(server.get('plugins', [])) or '<none>'}")
    print(f"    instances: {_instances_summary(server) or '<none>'}")

    run_ssh(server, f"mkdir -p {remote_root_q}", dry_run=dry_run)
    _rsync(server, render_dir, remote_root, dry_run=dry_run)

    if dry_run:
        print("=== Dry run complete; docker compose was not changed ===")
        return

    run_ssh(
        server,
        f"cd {remote_root_q} && docker compose pull && docker compose up -d --remove-orphans",
    )
    _check_services(server, remote_root_q)
    print(f"=== Deploy to {server_id} complete ===")


def tunnel_db(
    *,
    server_id: str | None,
    host_arg: str | None,
    ssh_user_arg: str | None,
    ssh_port_arg: str | None,
    db_host_arg: str | None,
    db_port_arg: str | None,
    local_port_arg: str | None,
    identity_arg: str | None,
) -> None:
    server = _load_optional_server(server_id)
    host = host_arg or (server and str(server["host"])) or os.environ.get("VPS_HOST", "")
    ssh_user = (
        ssh_user_arg or (server and str(server["ssh_user"])) or os.environ.get("SSH_USER", "steam")
    )
    ssh_port = (
        ssh_port_arg or (server and str(server["ssh_port"])) or os.environ.get("SSH_PORT", "22")
    )
    db_host = db_host_arg or os.environ.get("DB_HOST", "localhost")
    db_port = db_port_arg or os.environ.get("DB_PORT", "5432")
    local_port = local_port_arg or os.environ.get("LOCAL_PORT", "5433")
    identity = identity_arg or os.environ.get("SSH_KEY_FILE", "")

    if not host:
        die("VPS host is required (use --server <id> or --host <ip>)")

    ssh_args = [
        "ssh",
        "-N",
        "-p",
        str(ssh_port),
        "-L",
        f"127.0.0.1:{local_port}:{db_host}:{db_port}",
        "-o",
        "StrictHostKeyChecking=accept-new",
        "-o",
        "ServerAliveInterval=30",
        "-o",
        "ExitOnForwardFailure=yes",
    ]
    if identity:
        ssh_args += ["-i", identity, "-o", "IdentitiesOnly=yes"]
    ssh_args.append(f"{ssh_user}@{host}")

    print(
        f"=== SSH tunnel: 127.0.0.1:{local_port} -> {db_host}:{db_port} "
        f"(via {ssh_user}@{host}:{ssh_port}) ==="
    )
    print(
        "    connect: "
        f'psql "host=127.0.0.1 port={local_port} dbname=admin_system user=<db-user>"'
    )
    print("    stop:    Ctrl-C")
    run(ssh_args)


def ssh_options(server: dict[str, Any]) -> list[str]:
    options = [
        "-p",
        str(server["ssh_port"]),
        "-o",
        "StrictHostKeyChecking=accept-new",
    ]
    if os.environ.get("SSH_KEY_FILE"):
        options += ["-i", os.environ["SSH_KEY_FILE"], "-o", "IdentitiesOnly=yes"]
    return options


def ssh_target(server: dict[str, Any]) -> str:
    return f"{server['ssh_user']}@{server['host']}"


def run_ssh(
    server: dict[str, Any],
    remote_command: str,
    *,
    dry_run: bool = False,
    capture: bool = False,
) -> subprocess.CompletedProcess[str]:
    return run(
        ["ssh", *ssh_options(server), ssh_target(server), remote_command],
        dry_run=dry_run,
        capture=capture,
    )


def _load_optional_server(server_id: str | None) -> dict[str, Any] | None:
    if not server_id:
        return None
    server = inventory.find_server(inventory.load(), server_id)
    load_server_env(server_id, required=False)
    return server


def _instances_summary(server: dict[str, Any]) -> str:
    return " ".join(
        f"{item.get('name')}:{item.get('port', '')}:{item.get('map', '')}"
        for item in server.get("instances", [])
    )


def _rsync(server: dict[str, Any], render_dir: Path, remote_root: str, *, dry_run: bool) -> None:
    rsync_args = ["rsync", "-az"]
    if dry_run:
        rsync_args += ["--dry-run", "--verbose"]
    rsync_args += [
        "-e",
        command_line(["ssh", *ssh_options(server)]),
        f"{render_dir.as_posix()}/",
        f"{ssh_target(server)}:{remote_root}/",
    ]
    run(rsync_args)


def _check_services(server: dict[str, Any], remote_root_q: str) -> None:
    services = run_ssh(
        server,
        f"cd {remote_root_q} && docker compose ps --status running --services",
        capture=True,
    ).stdout.splitlines()
    running = set(services)

    failed = False
    for instance in server.get("instances", []):
        service = f"cs2-{instance['name']}"
        if service in running:
            print(f"    {service}: running")
            continue
        print(f"    ERROR: {service} is not running", file=sys.stderr)
        run_ssh(
            server,
            f"cd {remote_root_q} && docker compose logs --tail=80 {shlex.quote(service)}",
        )
        failed = True

    if failed:
        die(f"deploy to {server['id']} failed")
