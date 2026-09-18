"""The addons tree an instance runs."""

import json
import shutil
from pathlib import Path

from deploy.tools.addons.settings import SettingsRenderer
from deploy.tools.config.inventory import Inventory
from deploy.tools.config.server_env import ServerEnv
from deploy.tools.config.servers import Instance, Server
from deploy.tools.errors import DeployError
from deploy.tools.paths import PACKAGE


class AddonsBuilder:
    """Assembles an instance's `addons` tree from the host, the plugins and rendered settings."""

    # The framework's install component, staged under package/ by the same name.
    HOST = "host"
    # Metamod's manifest directory; since the single host, voltmod.vdf is the only manifest in it.
    MANIFEST_DIR = "metamod"
    HOST_MANIFEST = "metamod/voltmod.vdf"

    def __init__(self, inventory: Inventory, server: Server, env: ServerEnv) -> None:
        self._server = server
        self._settings = SettingsRenderer(inventory, server, env)

    @staticmethod
    def owned_paths(plugins: list[str]) -> list[str]:
        """What each plugin owns under addons/: its own folder, and nothing outside it."""
        return list(plugins)

    @classmethod
    def stale_manifests(cls, plugins: list[str]) -> list[str]:
        """The manifest names those plugins had before the host, relative to addons/metamod.

        Plugin names only, so a third-party Metamod plugin's manifest is left alone, and never
        the host's own manifest however a plugin is named.
        """
        host = cls.HOST_MANIFEST.rpartition("/")[2]
        return [f"{name}.vdf" for name in plugins if f"{name}.vdf" != host]

    def build(self, instance: Instance, destination: Path) -> Path:
        """Rebuild destination/addons with the host and the instance's plugins, and return it."""
        addons = destination / "addons"
        shutil.rmtree(addons, ignore_errors=True)
        addons.mkdir(parents=True)
        # The host is in every payload: it loads only plugins built against its own ABI.
        self._unpack(self.HOST, addons)
        if not (addons / self.HOST_MANIFEST).is_file():
            raise DeployError(
                f"package/{self.HOST} has no {self.HOST_MANIFEST}; "
                "rebuild the framework and run `uv run poe deploy-package`"
            )
        for plugin in self._server.plugins_for(instance):
            self._unpack(plugin, addons)
            settings = self._settings.render(instance, plugin)
            text = json.dumps(settings, indent=2, ensure_ascii=False) + "\n"
            settings_file = addons / plugin / "configs" / "settings.jsonc"
            settings_file.parent.mkdir(parents=True, exist_ok=True)
            settings_file.write_text(text, encoding="utf-8", newline="\n")
        return addons

    @staticmethod
    def _unpack(name: str, addons: Path) -> None:
        package = PACKAGE / name / "addons"
        if not package.is_dir():
            raise DeployError(f"no package for {name}; run `uv run poe deploy-package`")
        shutil.copytree(package, addons, dirs_exist_ok=True)
