"""Docker and Compose commands on a Docker host."""

import shlex
import subprocess
import sys
from pathlib import PurePosixPath

from deploy.tools.config.servers import DockerServer, Instance
from deploy.tools.docker.ssh import Ssh
from deploy.tools.errors import DeployError


class DockerHost:
    """Runs one host's Compose project and cleanup commands over SSH."""

    # A first start downloads the whole game.
    STEAMCMD_TIMEOUT_MINUTES = 90
    STEAMCMD_TIMEOUT_EXIT_CODE = 3

    def __init__(self, server: DockerServer, ssh: Ssh) -> None:
        self._server = server
        self._ssh = ssh
        self._root = shlex.quote(server.deploy_root)

    def create_folders(self, paths: list[str]) -> None:
        self._ssh.run(f"mkdir -p {shlex.join(paths)}")

    def remove(self, paths: list[str]) -> None:
        if paths:
            self._ssh.run(f"rm -rf -- {shlex.join(paths)}")

    def pull(self) -> None:
        self._compose("pull")

    def recreate(self, instance: Instance) -> None:
        """Start the instance in a new container even when nothing changed, so pre.sh runs."""
        self._compose(f"up -d --force-recreate {shlex.quote(instance.name)}")
        self._wait_for_steamcmd(instance)

    def restart(self, instance: Instance) -> None:
        self._compose(f"restart {shlex.quote(instance.name)}")
        self._wait_for_steamcmd(instance)

    def check_running(self) -> None:
        running = self._compose("ps --status running --services", capture=True).split()
        stopped = [item.name for item in self._server.instances if item.name not in running]
        for name in stopped:
            print(f"ERROR: {name} is not running; its last log lines follow", file=sys.stderr)
            self._compose(f"logs --tail=80 {shlex.quote(name)}")
        if stopped:
            raise DeployError(f"{', '.join(stopped)} did not start on {self._server.id}")
        print("    every instance is running")

    def remove_old_images(self, repository: str, keep: str) -> None:
        """Remove the repository's tags other than keep, since every deploy adds a tag."""
        image_format = "'{{.Repository}}:{{.Tag}}'"
        tags = f"docker image ls {shlex.quote(repository)} --format {image_format}"
        old_tags = f"{tags} | grep -vxF {shlex.quote(keep)} | xargs -r docker image rm"
        self._ssh.run(f"{old_tags} 2>/dev/null; docker image prune -f")

    def remove_deployment(self, repository: str) -> None:
        """Delete the stack, its containers and images, deploy_root and cs2_root."""
        folders = [self._server.deploy_root, self._server.cs2_root]
        for path in folders:
            posix = PurePosixPath(path)
            if not posix.is_absolute() or ".." in posix.parts or len(posix.parts) < 4:
                raise DeployError(f"refusing to delete {path}; use a deeper absolute path")
        names = [self._server.container_name(item) for item in self._server.instances]
        images = f"docker image ls -q {shlex.quote(repository)} | xargs -r docker image rm -f"
        commands = [
            f"if [ -f {self._root}/docker-compose.yml ]; then "
            f"(cd {self._root} && docker compose down --remove-orphans) || true; fi",
            f"docker rm -f {shlex.join(names)} 2>/dev/null || true",
            f"{images} 2>/dev/null || true",
            f"rm -rf -- {shlex.join(folders)}",
        ]
        self._ssh.run("; ".join(commands))

    def _compose(self, arguments: str, *, capture: bool = False) -> str:
        return self._ssh.run(f"cd {self._root} && docker compose {arguments}", capture=capture)

    def _wait_for_steamcmd(self, instance: Instance) -> None:
        """Block while SteamCMD runs in the instance's container, so updates go one at a time."""
        if self._ssh.dry_run:
            return
        service = shlex.quote(instance.name)
        waiting = shlex.quote(f"    {instance.name}: SteamCMD still running")
        limit = self.STEAMCMD_TIMEOUT_MINUTES * 60
        script = (
            f"cd {self._root} && container=$(docker compose ps -q {service}) && "
            'if [ -n "$container" ]; then sleep 10; waited=0; '
            "while docker exec \"$container\" sh -lc 'pgrep -f steamcmd >/dev/null 2>&1'; do "
            f"[ $waited -lt {limit} ] || exit {self.STEAMCMD_TIMEOUT_EXIT_CODE}; "
            f"echo {waiting}; sleep 15; waited=$((waited + 15)); done; fi"
        )
        try:
            self._ssh.run(script)
        except subprocess.CalledProcessError as error:
            if error.returncode != self.STEAMCMD_TIMEOUT_EXIT_CODE:
                raise
            minutes = self.STEAMCMD_TIMEOUT_MINUTES
            raise DeployError(f"SteamCMD in {instance.name} ran over {minutes} minutes") from None
