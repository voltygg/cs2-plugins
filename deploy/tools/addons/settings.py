"""A plugin's deployed settings.jsonc."""

import json
import re
from pathlib import Path
from typing import Any

from deploy.tools.config.inventory import Inventory
from deploy.tools.config.server_env import ServerEnv
from deploy.tools.config.servers import Instance, Server
from deploy.tools.errors import DeployError
from deploy.tools.paths import ROOT


class SettingsRenderer:
    """A plugin's own settings.jsonc with inventory overrides and its database filled in."""

    PLACEHOLDER = re.compile(r"\$\{([A-Za-z_][A-Za-z0-9_]*)\}")
    # Strings match first, so `//` inside a URL survives comment removal.
    COMMENT_OR_STRING = re.compile(r'"(?:\\.|[^"\\])*"|//[^\n]*|/\*.*?\*/', re.DOTALL)

    def __init__(self, inventory: Inventory, server: Server, env: ServerEnv) -> None:
        self._inventory = inventory
        self._server = server
        self._env = env

    def render(self, instance: Instance, plugin: str) -> dict[str, Any]:
        configs = ROOT / "plugins" / plugin / "configs"
        settings = self._read_jsonc(configs / "settings.jsonc")
        schema_file = configs / "settings.schema.json"
        schema = {}
        if schema_file.is_file():
            schema = json.loads(schema_file.read_text(encoding="utf-8"))
        variables = {
            **self._env.variables(),
            # Per-server admin grants reference the tag, so it must stay stable.
            "SERVER_TAG": f"{self._server.id}-{instance.name}",
            "SERVER_NAME": instance.server_name,
        }
        config = self._inventory.plugins[plugin]
        self._merge(settings, self._fill(config.settings, variables, plugin), schema, plugin)
        if config.database:
            database = self._database(config.database)
            settings["database"] = {**settings.get("database", {}), **database}
        return settings

    def _read_jsonc(self, path: Path) -> Any:
        if not path.is_file():
            raise DeployError(f"no plugin settings at {path.relative_to(ROOT).as_posix()}")
        text = self.COMMENT_OR_STRING.sub(self._keep_strings, path.read_text(encoding="utf-8"))
        return json.loads(text)

    @staticmethod
    def _keep_strings(match: re.Match[str]) -> str:
        return match.group(0) if match.group(0).startswith('"') else ""

    def _fill(self, value: Any, variables: dict[str, str], plugin: str) -> Any:
        """Replace ${NAME} in every string inside value; an unknown name fails."""
        if isinstance(value, dict):
            return {key: self._fill(item, variables, plugin) for key, item in value.items()}
        if isinstance(value, list):
            return [self._fill(item, variables, plugin) for item in value]
        if not isinstance(value, str):
            return value
        for name in self.PLACEHOLDER.findall(value):
            if name not in variables:
                raise DeployError(f"${{{name}}} in the {plugin} settings is not set")
        return self.PLACEHOLDER.sub(lambda match: variables[match.group(1)], value)

    def _merge(
        self,
        settings: dict[str, Any],
        overrides: dict[str, Any],
        schema: dict[str, Any],
        plugin: str,
        prefix: str = "",
    ) -> None:
        """Deep-merge overrides; lists and scalars replace. Keys the schema lacks fail."""
        known = schema.get("properties")
        for key, value in overrides.items():
            if known is not None and key not in known:
                raise DeployError(f"inventory sets unknown {plugin} setting '{prefix}{key}'")
            if not isinstance(value, dict):
                settings[key] = value
                continue
            if not isinstance(settings.get(key), dict):
                settings[key] = {}
            child_schema = (known or {}).get(key, {})
            self._merge(settings[key], value, child_schema, plugin, f"{prefix}{key}.")

    def _database(self, name: str) -> dict[str, Any]:
        database = self._inventory.database
        host = self._env.get("DB_HOST", database.host or "")
        if not host:
            raise DeployError(f"DB_HOST is not set for {self._server.id}")
        return {
            "host": host,
            "port": database.port,
            "database": name,
            "username": database.user,
            "password": self._env.require("DB_PASSWORD"),
            "sslMode": database.ssl_mode,
        }
