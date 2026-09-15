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
    """Assembles an instance's `addons` tree from plugin packages and rendered settings."""

    def __init__(self, inventory: Inventory, server: Server, env: ServerEnv) -> None:
        self._server = server
        self._settings = SettingsRenderer(inventory, server, env)

    @staticmethod
    def owned_paths(plugins: list[str]) -> list[str]:
        """What each plugin owns under addons/: its folder and its Metamod entry."""
        return [path for name in plugins for path in (name, f"metamod/{name}.vdf")]

    def build(self, instance: Instance, destination: Path) -> Path:
        """Rebuild destination/addons and return it."""
        addons = destination / "addons"
        shutil.rmtree(addons, ignore_errors=True)
        addons.mkdir(parents=True)
        for plugin in self._server.plugins_for(instance):
            package = PACKAGE / plugin / "addons"
            if not package.is_dir():
                raise DeployError(f"no package for {plugin}; run `uv run poe deploy-package`")
            shutil.copytree(package, addons, dirs_exist_ok=True)
            settings = self._settings.render(instance, plugin)
            text = json.dumps(settings, indent=2, ensure_ascii=False) + "\n"
            settings_file = addons / plugin / "configs" / "settings.jsonc"
            settings_file.parent.mkdir(parents=True, exist_ok=True)
            settings_file.write_text(text, encoding="utf-8", newline="\n")
        return addons
