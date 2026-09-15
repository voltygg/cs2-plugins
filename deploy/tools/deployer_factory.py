"""Builds the deployer for each inventory server."""

from typing import Any

from deploy.tools.config.inventory import Inventory
from deploy.tools.config.servers import DockerServer, PanelServer
from deploy.tools.deployer import Deployer
from deploy.tools.docker.deployer import DockerDeployer
from deploy.tools.errors import DeployError
from deploy.tools.panel.deployer import PanelDeployer


class DeployerFactory:
    """Creates the Deployer subclass that matches each server's kind."""

    BY_KIND: dict[str, type[Deployer[Any]]] = {"docker": DockerDeployer, "panel": PanelDeployer}

    def __init__(self) -> None:
        self.inventory = Inventory.load()

    def for_servers(self, server_id: str | None, *, dry_run: bool = False) -> list[Deployer[Any]]:
        """Deployers for the named server, or for every enabled one."""
        return [self._create(server, dry_run) for server in self.inventory.select(server_id)]

    def for_server(self, server_id: str | None, *, dry_run: bool = False) -> Deployer[Any]:
        """The named server's deployer, or the only enabled server's."""
        servers = self.inventory.select(server_id)
        if len(servers) != 1:
            ids = ", ".join(server.id for server in servers) or "none enabled"
            raise DeployError(f"pass --server ({ids})")
        return self._create(servers[0], dry_run)

    def _create(self, server: DockerServer | PanelServer, dry_run: bool) -> Deployer[Any]:
        return self.BY_KIND[server.kind](self.inventory, server, dry_run=dry_run)
