"""The Compose project a Docker host runs."""

import json
from pathlib import Path
from typing import Any

import yaml

from deploy.tools.addons.builder import AddonsBuilder
from deploy.tools.config.server_env import ServerEnv
from deploy.tools.config.servers import DockerServer, Instance
from deploy.tools.paths import DEPLOY


class ComposeProject:
    """docker-compose.yml plus each instance's plugin bundle, env file and pre-launch hook."""

    # Where the runtime image expects the CS2 install.
    GAME_ROOT = "/home/steam/cs2-dedicated"
    PRE_LAUNCH_HOOK = DEPLOY / "docker" / "pre.sh"

    def __init__(
        self, server: DockerServer, env: ServerEnv, addons: AddonsBuilder, image: str
    ) -> None:
        self._server = server
        self._env = env
        self._addons = addons
        self._image = image

    def render(self, destination: Path) -> None:
        for instance in self._server.instances:
            instance_dir = destination / "instances" / instance.name
            self._addons.build(instance, instance_dir / "bundles")
            env_file = instance_dir / ".env"
            env_file.write_text(self._env_file(instance), encoding="utf-8", newline="\n")
            hook = instance_dir / "pre.sh"
            hook_text = self.PRE_LAUNCH_HOOK.read_text(encoding="utf-8")
            hook.write_text(hook_text, encoding="utf-8", newline="\n")
            hook.chmod(0o755)
        compose = yaml.safe_dump(self._compose_file(), sort_keys=False, allow_unicode=True)
        (destination / "docker-compose.yml").write_text(compose, encoding="utf-8", newline="\n")

    def _compose_file(self) -> dict[str, Any]:
        services = {}
        for instance in self._server.instances:
            local = f"./instances/{instance.name}"
            port = instance.port
            services[instance.name] = {
                "image": self._image,
                "container_name": self._server.container_name(instance),
                "restart": "unless-stopped",
                "env_file": [f"{local}/.env"],
                "ports": [f"{port}:{port}/udp", f"{port}:{port}/tcp"],
                # Reaches Postgres on the host from the bridge network.
                "extra_hosts": ["host.docker.internal:host-gateway"],
                "volumes": [
                    f"{self._server.game_install}:{self.GAME_ROOT}",
                    # Each instance mounts its own addons over the shared install.
                    f"{local}/addons:{self.GAME_ROOT}/game/csgo/addons",
                    f"{local}/pre.sh:{self.GAME_ROOT}/pre.sh:ro",
                    f"{local}/bundles:/home/steam/plugin-bundles:ro",
                ],
            }
        return {"name": "cs2", "services": services}

    def _env_file(self, instance: Instance) -> str:
        token = self._env.get(f"GSLT_{instance.name}")
        if not token:
            print(f"WARNING: GSLT_{instance.name} is not set; '{instance.name}' starts in LAN mode")
        values = {
            "SRCDS_TOKEN": token,
            "CS2_RCONPW": self._env.get("RCON_PASSWORD"),
            "CS2_PORT": str(instance.port),
            "CS2_STARTMAP": instance.map,
            "CS2_SERVERNAME": instance.server_name,
        }
        # No +exec server.cfg here: it can turn VAC back on after -insecure.
        if instance.insecure:
            values["CS2_ADDITIONAL_ARGS"] = "-insecure"
        return "".join(f"{key}={json.dumps(value)}\n" for key, value in values.items())
