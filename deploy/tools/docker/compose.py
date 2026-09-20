"""The Compose project a Docker host runs."""

import shlex
from pathlib import Path

from deploy.tools.addons.builder import AddonsBuilder
from deploy.tools.config.server_env import ServerEnv
from deploy.tools.config.servers import DockerServer, Instance
from deploy.tools.paths import DEPLOY


class ComposeProject:
    """docker-compose.yml plus each instance's plugin bundle, env file and pre-launch hook."""

    COMPOSE_FILE = DEPLOY / "docker" / "docker-compose.yml"
    PRE_LAUNCH_HOOK = DEPLOY / "docker" / "pre.sh"

    def __init__(
        self, server: DockerServer, env: ServerEnv, addons: AddonsBuilder, image: str
    ) -> None:
        self._server = server
        self._env = env
        self._addons = addons
        self._image = image

    def render(self, destination: Path) -> None:
        hook_text = self.PRE_LAUNCH_HOOK.read_text(encoding="utf-8")
        for instance in self._server.instances:
            instance_dir = destination / "instances" / instance.name
            self._addons.build(instance, instance_dir / "bundles")
            self._write(instance_dir / ".env", self._env_file(instance))
            hook = instance_dir / "pre.sh"
            self._write(hook, hook_text)
            hook.chmod(0o755)
        compose_text = self.COMPOSE_FILE.read_text(encoding="utf-8")
        self._write(destination / self.COMPOSE_FILE.name, compose_text)

    @staticmethod
    def _write(path: Path, text: str) -> None:
        """The host reads these files, so they keep LF endings whatever the renderer runs on."""
        path.write_text(text, encoding="utf-8", newline="\n")

    def _env_file(self, instance: Instance) -> str:
        token = self._env.get(f"GSLT_{instance.name}")
        if not token:
            print(f"WARNING: GSLT_{instance.name} is not set; '{instance.name}' starts in LAN mode")
        values = {
            # Read by docker-compose.yml; the rest configures the container.
            "INSTANCE": instance.name,
            "CONTAINER_NAME": self._server.container_name(instance),
            "RUNTIME_IMAGE": self._image,
            "GAME_INSTALL": self._server.game_install,
            "SRCDS_TOKEN": token,
            "CS2_RCONPW": self._env.get("RCON_PASSWORD"),
            "CS2_PORT": str(instance.port),
            "CS2_STARTMAP": instance.map,
            "CS2_SERVERNAME": instance.server_name,
        }
        # No +exec server.cfg here: it can turn VAC back on after -insecure.
        if instance.insecure:
            values["CS2_ADDITIONAL_ARGS"] = "-insecure"
        # Quoted: Compose expands an unquoted $ when it interpolates the file.
        return "".join(f"{key}={shlex.quote(value)}\n" for key, value in values.items())
