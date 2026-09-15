"""The operations every server kind supports."""

from abc import ABC, abstractmethod
from contextlib import AbstractContextManager

from deploy.tools.addons.builder import AddonsBuilder
from deploy.tools.config.inventory import Inventory
from deploy.tools.config.server_env import ServerEnv
from deploy.tools.config.servers import Instance, Server
from deploy.tools.errors import DeployError


class Deployer[S: Server](ABC):
    """Deploys and operates one inventory server."""

    def __init__(self, inventory: Inventory, server: S, *, dry_run: bool = False) -> None:
        self.inventory = inventory
        self.server = server
        self.env = ServerEnv(server.id)
        self.dry_run = dry_run
        self.addons = AddonsBuilder(inventory, server, self.env)

    @abstractmethod
    def deploy(self) -> None:
        """Install the server's plugins and settings, then restart it."""

    @abstractmethod
    def update(self) -> None:
        """Restart the server so it installs the latest CS2 build."""

    @abstractmethod
    def rcon_address(self, instance: Instance) -> AbstractContextManager[tuple[str, int]]:
        """The host and port this machine reaches the instance's RCON on, while the block runs."""

    def cleanup(self) -> None:
        raise DeployError(f"cleanup needs SSH; '{self.server.id}' is a {self.server.kind} server")

    def tunnel_database(self, local_port: int, db_host: str, db_port: int) -> None:
        raise DeployError(f"a tunnel needs SSH; '{self.server.id}' is a {self.server.kind} server")

    def unused_plugin_paths(self, instance: Instance) -> list[str]:
        """Paths under addons/ owned by inventory plugins the instance does not run."""
        return AddonsBuilder.owned_paths(self.inventory.unused_plugins(self.server, instance))
