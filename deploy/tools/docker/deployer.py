import os
from collections.abc import Generator
from contextlib import contextmanager

from deploy.tools import console
from deploy.tools.addons.builder import AddonsBuilder
from deploy.tools.config.inventory import Inventory
from deploy.tools.config.servers import DockerServer, Instance
from deploy.tools.deployer import Deployer
from deploy.tools.docker.compose import ComposeProject
from deploy.tools.docker.host import DockerHost
from deploy.tools.docker.ssh import Ssh
from deploy.tools.paths import RENDER_DIR


class DockerDeployer(Deployer[DockerServer]):
    """Syncs a rendered Compose project to the host and recreates instances one at a time."""

    def __init__(
        self, inventory: Inventory, server: DockerServer, *, dry_run: bool = False
    ) -> None:
        super().__init__(inventory, server, dry_run=dry_run)
        self.ssh = Ssh(server, self.secrets.get("SSH_KEY_FILE"), dry_run=dry_run)
        self.host = DockerHost(server, self.ssh)
        self.project = ComposeProject(server, self.secrets, self.addons, self.image)

    @property
    def image(self) -> str:
        """The runtime image at RUNTIME_IMAGE_TAG, which CI sets to the commit SHA."""
        return f"{self.inventory.runtime_image}:{os.environ.get('RUNTIME_IMAGE_TAG', 'latest')}"

    def deploy(self) -> None:
        render_dir = RENDER_DIR / self.server.id
        self.project.render(render_dir)
        root = self.server.deploy_root
        self.print_plan(f"Deploying to {self.ssh.target}:{root} for", f"image: {self.image}")

        # Bind-mount sources must exist first, or Docker creates them as root.
        addons_dirs = [f"{self.server.instance_dir(item)}/addons" for item in self.server.instances]
        self.host.create_folders([root, self.server.game_install, *addons_dirs])
        self.ssh.sync(render_dir, root)
        unused = [path for item in self.server.instances for path in self._unused_paths(item)]
        self.host.remove(unused)

        if self.dry_run:
            console.done("Dry run complete; no container changed")
            return

        self.host.pull()
        for instance in self.server.instances:
            self.host.recreate(instance)
        self.host.check_running()
        self.host.remove_old_images(self.inventory.runtime_image, keep=self.image)
        console.done(f"Deploy to {self.server.id} complete")

    def restart(self) -> None:
        self.print_plan("Restarting")
        if self.dry_run:
            console.done("Dry run complete; nothing restarted")
            return
        for instance in self.server.instances:
            self.host.restart(instance)
        self.host.check_running()
        console.done(f"Restart of {self.server.id} complete")

    def cleanup(self) -> None:
        console.section(f"Removing the deployment from {self.server.id} ({self.ssh.target})")
        self.host.remove_deployment(self.inventory.runtime_image)
        console.done(f"Cleanup of {self.server.id} complete")

    def tunnel_database(self, local_port: int, db_host: str, db_port: int) -> None:
        with self.ssh.tunnel(db_host, db_port, local_port) as tunnel:
            console.section(f"127.0.0.1:{local_port} -> {db_host}:{db_port} on {self.ssh.target}")
            console.item(f'psql "host=127.0.0.1 port={local_port} dbname=<database> user=postgres"')
            console.item("Ctrl-C closes the tunnel")
            tunnel.wait()

    @contextmanager
    def rcon_address(self, instance: Instance) -> Generator[tuple[str, int]]:
        # The host firewall opens only the game's UDP port, so RCON goes through SSH.
        with self.ssh.tunnel("127.0.0.1", instance.port) as tunnel:
            yield "127.0.0.1", tunnel.local_port

    def _unused_paths(self, instance: Instance) -> list[str]:
        """Unused plugin folders in both trees: the sync and pre.sh only ever add files."""
        instance_dir = self.server.instance_dir(instance)
        trees = ("bundles/addons", "addons")
        unused = self.unused_plugin_paths(instance)
        return [
            f"{instance_dir}/{tree}/{AddonsBuilder.PLUGINS_DIR}/{path}"
            for tree in trees
            for path in unused
        ]
