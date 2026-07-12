"""Remote SSH operations for Docker deploy hosts."""

from __future__ import annotations

import os
import shlex
import socket
import subprocess
import sys
import time
from pathlib import Path, PurePosixPath
from typing import Any

from . import inventory, render
from .common import DEPLOY, command_line, die, load_server_env, repo_path, run


def deploy_server(
    server_id: str,
    package_dir: str,
    runtime_image: str | None,
    *,
    dry_run: bool,
) -> None:
    """Render and apply one server's Docker Compose tree over SSH."""
    server = inventory.find_server(inventory.load(), server_id)
    load_server_env(server_id, required=True)

    render_dir = DEPLOY / ".render" / server_id
    render.render(server_id, repo_path(package_dir), render_dir, runtime_image)

    remote_root = str(server["deploy_root"])
    remote_root_q = shlex.quote(remote_root)
    target = ssh_target(server)

    print(f"=== Deploying Dockerized CS2 to {server_id} ({target}:{remote_root}) ===")
    print(f"    default plugins: {' '.join(server.get('plugins', [])) or '<none>'}")
    print(f"    instances:       {_instances_summary(server) or '<none>'}")

    cs2_root = str(server["cs2_root"]).rstrip("/")
    # pre-create addons dirs as the steam user so Docker doesn't make them root
    mkdir_paths = [remote_root, f"{cs2_root}/server"]
    mkdir_paths += [
        f"{remote_root}/instances/{instance['name']}/addons"
        for instance in server.get("instances", [])
    ]
    run_ssh(
        server,
        "mkdir -p " + " ".join(shlex.quote(path) for path in mkdir_paths),
        dry_run=dry_run,
    )
    _rsync(server, render_dir, remote_root, dry_run=dry_run)

    if dry_run:
        print("=== Dry run complete; docker compose was not changed ===")
        return

    run_ssh(server, f"cd {remote_root_q} && docker compose pull")
    for instance in server.get("instances", []):
        _compose_service(server, remote_root_q, "up -d", str(instance["name"]))
    _check_services(server, remote_root_q)
    # Each pull of :latest strands the previous digest as a dangling <none> image (multi-GB
    # runtime layers). Prune only after the new containers are confirmed healthy.
    run_ssh(server, "docker image prune -f")
    print(f"=== Deploy to {server_id} complete ===")


def update_server(server_id: str, *, dry_run: bool) -> None:
    """Restart one server's instances so SteamCMD pulls the latest CS2 build.

    The runtime image runs SteamCMD only at container start, so a long-running
    container never picks up Valve updates. Restarting re-execs the entrypoint
    and updates the shared install in the cs2_root volume.
    """
    server = inventory.find_server(inventory.load(), server_id)
    load_server_env(server_id, required=False)

    remote_root_q = shlex.quote(str(server["deploy_root"]))
    target = ssh_target(server)

    print(f"=== Updating CS2 on {server_id} ({target}) ===")
    print(f"    instances: {_instances_summary(server) or '<none>'}")

    # Sequential: instances share one CS2 install; concurrent SteamCMD writes
    # to the same volume must be avoided.
    for instance in server.get("instances", []):
        _compose_service(server, remote_root_q, "restart", str(instance["name"]), dry_run=dry_run)

    if dry_run:
        print("=== Dry run complete; no containers were restarted ===")
        return

    _check_services(server, remote_root_q)
    print(f"=== Update for {server_id} complete ===")


def cleanup_server(server_id: str, *, yes: bool, dry_run: bool) -> None:
    """Remove one server's Docker stack, deploy files, CS2 files, and images."""
    if not yes and not dry_run:
        die("cleanup is destructive; pass --yes or use --dry-run")

    server = inventory.find_server(inventory.load(), server_id)
    remote_root = str(server["deploy_root"])
    cs2_root = str(server["cs2_root"])
    _require_safe_remote_path(remote_root, "deploy_root")
    _require_safe_remote_path(cs2_root, "cs2_root")

    container_names = [f"{server['id']}-cs2-{item['name']}" for item in server.get("instances", [])]
    runtime_image = str(server.get("runtime_image", ""))
    remote_root_q = shlex.quote(remote_root)
    cs2_root_q = shlex.quote(cs2_root)
    runtime_image_q = shlex.quote(runtime_image)
    container_args = " ".join(shlex.quote(name) for name in container_names)

    print(f"=== Cleaning Dockerized CS2 from {server_id} ===")
    print(f"    deploy_root: {remote_root}")
    print(f"    cs2_root:    {cs2_root}")
    print(f"    containers:  {' '.join(container_names) or '<none>'}")
    print(f"    image:       {runtime_image or '<none>'}")

    commands = [
        "set -u",
        (
            f"if [ -f {remote_root_q}/docker-compose.yml ]; then "
            f"(cd {remote_root_q} || exit 1; "
            "docker compose config --images > .cleanup-images 2>/dev/null || true; "
            "docker compose down --remove-orphans || true; "
            "if [ -s .cleanup-images ]; then "
            "xargs -r docker image rm -f < .cleanup-images || true; "
            "fi); "
            "fi"
        ),
    ]
    if container_args:
        commands.append(f"docker rm -f {container_args} 2>/dev/null || true")
    if runtime_image:
        commands.append(f"docker image rm -f {runtime_image_q} 2>/dev/null || true")
    commands.append(f"rm -rf -- {cs2_root_q}")

    run_ssh(server, "; ".join(commands), dry_run=dry_run)
    print(f"=== Cleanup for {server_id} complete ===")


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
    """Open an SSH local port forward to a server-side Postgres port."""
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

    endpoint = {"host": host, "ssh_user": ssh_user, "ssh_port": ssh_port}
    ssh_args = forward_args(
        endpoint,
        local_port,
        db_host,
        db_port,
        identity=identity or None,
        extra_options=("-o", "ServerAliveInterval=30"),
    )

    print(
        f"=== SSH tunnel: 127.0.0.1:{local_port} -> {db_host}:{db_port} "
        f"(via {ssh_user}@{host}:{ssh_port}) ==="
    )
    print(
        f'    connect: psql "host=127.0.0.1 port={local_port} dbname=admin_system user=<db-user>"'
    )
    print("    stop:    Ctrl-C")
    run(ssh_args)


def ssh_options(server: dict[str, Any], *, identity: str | None = None) -> list[str]:
    """Return common ssh options for a resolved inventory server."""
    options = [
        "-p",
        str(server["ssh_port"]),
        "-o",
        "StrictHostKeyChecking=accept-new",
    ]
    identity = identity or os.environ.get("SSH_KEY_FILE")
    if identity:
        options += ["-i", identity, "-o", "IdentitiesOnly=yes"]
    return options


def forward_args(
    server: dict[str, Any],
    local_port: int | str,
    dest_host: str,
    dest_port: int | str,
    *,
    identity: str | None = None,
    extra_options: tuple[str, ...] = (),
) -> list[str]:
    """Build the `ssh -N` local port-forward command for a server-like dict."""
    return [
        "ssh",
        "-N",
        *ssh_options(server, identity=identity),
        "-o",
        "ExitOnForwardFailure=yes",
        *extra_options,
        "-L",
        f"127.0.0.1:{local_port}:{dest_host}:{dest_port}",
        ssh_target(server),
    ]


def open_tunnel(
    server: dict[str, Any], dest_host: str, dest_port: int
) -> tuple[subprocess.Popen[bytes], int]:
    """Start a background forward from a free local port and wait until it accepts."""
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        local_port = probe.getsockname()[1]

    args = forward_args(
        server, local_port, dest_host, dest_port, extra_options=("-o", "BatchMode=yes")
    )
    tunnel = subprocess.Popen(args)

    deadline = time.monotonic() + 15.0
    while time.monotonic() < deadline:
        if tunnel.poll() is not None:
            die(f"SSH tunnel to {server['id']} exited with code {tunnel.returncode}")
        try:
            socket.create_connection(("127.0.0.1", local_port), timeout=1.0).close()
            return tunnel, local_port
        except OSError:
            time.sleep(0.3)

    tunnel.terminate()
    die(f"SSH tunnel to {server['id']} did not come up within 15s")


def ssh_target(server: dict[str, Any]) -> str:
    """Return user@host for a resolved inventory server."""
    return f"{server['ssh_user']}@{server['host']}"


def run_ssh(
    server: dict[str, Any],
    remote_command: str,
    *,
    dry_run: bool = False,
    capture: bool = False,
) -> subprocess.CompletedProcess[str]:
    """Run a remote command on a resolved inventory server."""
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
        f"{item.get('name')}:{item.get('port', '')}"
        f"[{','.join(inventory.instance_plugins(server, item)) or '-'}]"
        for item in server.get("instances", [])
    )


def _compose_service(
    server: dict[str, Any], remote_root_q: str, action: str, service: str, *, dry_run: bool = False
) -> None:
    """Run one docker compose action ("up -d" or "restart") and wait for SteamCMD."""
    service_q = shlex.quote(service)
    run_ssh(server, f"cd {remote_root_q} && docker compose {action} {service_q}", dry_run=dry_run)
    if not dry_run:
        _wait_for_steamcmd(server, remote_root_q, service)


def _wait_for_steamcmd(server: dict[str, Any], remote_root_q: str, service: str) -> None:
    service_q = shlex.quote(service)
    wait_msg_q = shlex.quote(f"    {service}: steamcmd still running; waiting")
    done_msg_q = shlex.quote(f"    {service}: steamcmd idle")
    run_ssh(
        server,
        (
            f"cd {remote_root_q} && "
            f"container=$(docker compose ps -q {service_q}) && "
            'if [ -n "$container" ]; then '
            "sleep 10; "
            'while docker exec "$container" sh -lc '
            "'pgrep -f steamcmd >/dev/null 2>&1'; do "
            f"echo {wait_msg_q}; "
            "sleep 15; "
            "done; "
            f"echo {done_msg_q}; "
            "fi"
        ),
    )


def _require_safe_remote_path(path: str, name: str) -> None:
    parsed = PurePosixPath(path)
    if not parsed.is_absolute() or ".." in parsed.parts or len(parsed.parts) < 4:
        die(f"unsafe {name}: {path}")


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
        service = str(instance["name"])
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
