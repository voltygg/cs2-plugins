"""Source RCON client for deployed CS2 instances.

Resolves host/port from deploy/inventory.yml and the password from the server's
.env (RCON_<instance>), so a live server can be driven straight from the repo:

    uv run poe rcon status
    uv run poe rcon "sv_autobunnyhopping" "bhop_reload"
    uv run poe rcon --server box-a --instance main "changelevel de_dust2"

Any convar read, plugin command, or map change works - the fastest way to test
a hypothesis against the running server without a deploy cycle.

The RCON TCP port is not reachable from outside the box (the in-game `rcon`
command rides the game connection instead), so commands run through a short-
lived SSH port forward using the same key the deploy tooling uses.
"""

from __future__ import annotations

import os
import socket
import struct
import subprocess
import time
from typing import Any

import inventory
from common import die, load_server_env

_SERVERDATA_AUTH = 3
_SERVERDATA_AUTH_RESPONSE = 2  # shares the value with EXECCOMMAND (protocol quirk)
_SERVERDATA_EXECCOMMAND = 2
_SERVERDATA_RESPONSE_VALUE = 0
_AUTH_FAILED_ID = -1


def _send_packet(sock: socket.socket, packet_id: int, packet_type: int, body: str) -> None:
    data = struct.pack("<ii", packet_id, packet_type) + body.encode() + b"\x00\x00"
    sock.sendall(struct.pack("<i", len(data)) + data)


def _recv_exact(sock: socket.socket, size: int) -> bytes:
    data = b""
    while len(data) < size:
        chunk = sock.recv(size - len(data))
        if not chunk:
            raise ConnectionError("RCON connection closed by server")
        data += chunk
    return data


def _recv_packet(sock: socket.socket) -> tuple[int, int, str]:
    (size,) = struct.unpack("<i", _recv_exact(sock, 4))
    data = _recv_exact(sock, size)
    packet_id, packet_type = struct.unpack("<ii", data[:8])
    return packet_id, packet_type, data[8:-2].decode(errors="replace")


class RconClient:
    """Minimal Source RCON protocol client (TCP)."""

    def __init__(self, host: str, port: int, password: str, timeout: float = 10.0) -> None:
        self._sock = socket.create_connection((host, port), timeout=timeout)
        self._sock.settimeout(timeout)
        self._next_id = 10
        self._auth(password)

    def _auth(self, password: str) -> None:
        _send_packet(self._sock, 1, _SERVERDATA_AUTH, password)
        while True:
            packet_id, packet_type, _ = _recv_packet(self._sock)
            if packet_type != _SERVERDATA_AUTH_RESPONSE:
                continue
            if packet_id == _AUTH_FAILED_ID:
                die("RCON authentication failed (check RCON_<instance> in the server .env)")
            return

    def execute(self, command: str) -> str:
        """Run one command and return its full (possibly multi-packet) response."""
        command_id = self._next_id
        self._next_id += 2
        _send_packet(self._sock, command_id, _SERVERDATA_EXECCOMMAND, command)
        # CS2 captures a command's console output asynchronously: if the end-marker arrives in
        # the same TCP segment as the command (typical over an SSH tunnel), the server answers
        # the marker before any output is flushed and the response reads empty. Give the
        # capture window a beat before sending the marker.
        time.sleep(0.25)
        # Empty RESPONSE_VALUE sentinel: the server echoes it after the command's
        # response packets, marking the end of a multi-packet response.
        _send_packet(self._sock, command_id + 1, _SERVERDATA_RESPONSE_VALUE, "")

        parts: list[str] = []
        while True:
            packet_id, _, body = _recv_packet(self._sock)
            if packet_id == command_id + 1:
                return "".join(parts).strip()
            parts.append(body)

    def close(self) -> None:
        self._sock.close()


def _resolve_target(
    server_id: str | None, instance_name: str | None
) -> tuple[dict[str, Any], int, str]:
    """Resolve (server, rcon port, password) for one inventory instance."""
    data = inventory.load()
    servers = inventory.active_servers(data)
    if server_id:
        server = inventory.find_server(data, server_id)
    elif len(servers) == 1:
        server = servers[0]
    else:
        die(f"multiple servers in inventory; pass --server ({', '.join(s['id'] for s in servers)})")

    instances: list[dict[str, Any]] = server.get("instances", [])
    if instance_name:
        matches = [item for item in instances if item.get("name") == instance_name]
        if not matches:
            die(f"instance '{instance_name}' not found on server '{server['id']}'")
        target = matches[0]
    elif len(instances) == 1:
        target = instances[0]
    else:
        die(f"multiple instances on '{server['id']}'; pass --instance")

    load_server_env(str(server["id"]), required=True)
    password = os.environ.get(f"RCON_{target['name']}", "")
    if not password:
        die(f"RCON_{target['name']} not set in the '{server['id']}' .env")

    return server, int(target["port"]), password


def _open_tunnel(server: dict[str, Any], remote_port: int) -> tuple[subprocess.Popen[bytes], int]:
    """Forward a free local port to the instance's RCON port on the box."""
    import remote

    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        local_port = probe.getsockname()[1]

    ssh_args = [
        "ssh",
        "-N",
        *remote.ssh_options(server),
        "-o",
        "ExitOnForwardFailure=yes",
        "-o",
        "BatchMode=yes",
        "-L",
        f"127.0.0.1:{local_port}:127.0.0.1:{remote_port}",
        remote.ssh_target(server),
    ]
    tunnel = subprocess.Popen(ssh_args)

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
    raise AssertionError("unreachable")


def run_commands(
    commands: list[str], *, server_id: str | None = None, instance_name: str | None = None
) -> None:
    """Execute commands sequentially against one instance, printing each response."""
    server, rcon_port, password = _resolve_target(server_id, instance_name)
    tunnel, local_port = _open_tunnel(server, rcon_port)
    try:
        client = RconClient("127.0.0.1", local_port, password)
        try:
            for command in commands:
                response = client.execute(command)
                print(f"### {command}")
                if response:
                    print(response)
        finally:
            client.close()
    finally:
        tunnel.terminate()
