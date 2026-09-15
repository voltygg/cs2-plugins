"""ssh and rsync against a Docker host."""

import shlex
import socket
import subprocess
from pathlib import Path

from deploy.tools.config.servers import DockerServer
from deploy.tools.docker.tunnel import Tunnel


class Ssh:
    """ssh and rsync against one Docker host. A dry run prints remote commands instead."""

    def __init__(self, server: DockerServer, identity: str, *, dry_run: bool) -> None:
        self.target = f"{server.ssh_user}@{server.host}"
        self.dry_run = dry_run
        self._options = ["-p", str(server.ssh_port), "-o", "StrictHostKeyChecking=accept-new"]
        if identity:
            self._options += ["-i", identity, "-o", "IdentitiesOnly=yes"]

    def run(self, command: str, *, capture: bool = False) -> str:
        args = ["ssh", *self._options, self.target, command]
        if self.dry_run:
            print(f"DRY: {shlex.join(args)}")
            return ""
        return subprocess.run(args, check=True, text=True, capture_output=capture).stdout or ""

    def sync(self, source: Path, destination: str) -> None:
        """Copy source into destination without deleting extra files, so installed Metamod stays."""
        preview = ["--dry-run", "--verbose"] if self.dry_run else []
        shell = shlex.join(["ssh", *self._options])
        paths = [f"{source.as_posix()}/", f"{self.target}:{destination}/"]
        subprocess.run(["rsync", "-az", *preview, "-e", shell, *paths], check=True)

    def tunnel(self, remote_host: str, remote_port: int, local_port: int = 0) -> Tunnel:
        """Forward local_port, or any free port for 0, to remote_host:remote_port on the host."""
        if not local_port:
            with socket.socket() as probe:
                probe.bind(("127.0.0.1", 0))
                local_port = probe.getsockname()[1]
        forward = f"127.0.0.1:{local_port}:{remote_host}:{remote_port}"
        options = ["-o", "ExitOnForwardFailure=yes", "-o", "ServerAliveInterval=30"]
        command = ["ssh", "-N", *self._options, *options, "-L", forward, self.target]
        return Tunnel(command, local_port)
