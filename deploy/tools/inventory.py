#!/usr/bin/env python3
"""Inventory helpers for Dockerized CS2 deploys."""

from __future__ import annotations

import os
from pathlib import Path
from typing import Any

import yaml

from .common import DEPLOY, die

INVENTORY_PATH = Path(os.environ.get("INVENTORY_PATH", DEPLOY / "inventory.yml"))
_INHERITED = ("ssh_user", "ssh_port", "cs2_root", "deploy_root", "runtime_image")
SERVER_KINDS = ("docker", "pterodactyl")


def load(path: Path | str | None = None) -> dict[str, Any]:
    """Load the deploy inventory YAML."""
    with Path(path or INVENTORY_PATH).open(encoding="utf-8") as fh:
        data = yaml.safe_load(fh) or {}
    if "servers" not in data:
        die(f"no 'servers' in {path or INVENTORY_PATH}")
    return data


def active_servers(data: dict[str, Any]) -> list[dict[str, Any]]:
    """Return enabled servers with defaults resolved."""
    return [
        resolve_server(data, server)
        for server in data.get("servers", [])
        if server.get("enabled", True)
    ]


def find_server(data: dict[str, Any], server_id: str) -> dict[str, Any]:
    """Find one server by id and resolve inherited defaults."""
    for server in data.get("servers", []):
        if server.get("id") == server_id:
            return resolve_server(data, server)
    die(f"server '{server_id}' not found in inventory.yml")


def resolve_server(data: dict[str, Any], server: dict[str, Any]) -> dict[str, Any]:
    """Merge inventory defaults into one server record."""
    defaults = data.get("defaults", {})
    resolved = {"id": server.get("id"), "host": server.get("host")}
    resolved.update({key: server.get(key, defaults.get(key)) for key in _INHERITED})
    resolved["kind"] = server.get("kind", "docker")
    if resolved["kind"] not in SERVER_KINDS:
        die(f"server '{resolved['id']}' has unknown kind '{resolved['kind']}'")
    if resolved["kind"] == "pterodactyl":
        resolved["panel_url"] = server.get("panel_url")
        resolved["game_dir"] = server.get("game_dir", "/game/csgo")
    resolved["environment"] = server.get("environment")
    resolved["enabled"] = bool(server.get("enabled", True))
    resolved["plugins"] = list(server.get("plugins", []))
    instances = list(server.get("instances", []))
    for instance in instances:
        if not instance.get("name") or not instance.get("port"):
            die(f"server '{resolved['id']}' has an instance without name/port")
    resolved["instances"] = instances
    return resolved


def used_plugins(data: dict[str, Any]) -> list[str]:
    """Return every declared plugin name."""
    return list(data.get("plugins", {}).keys())


def instance_plugins(server: dict[str, Any], instance: dict[str, Any]) -> list[str]:
    """Return the plugins for one instance: its own list, else the server default."""
    plugins = instance.get("plugins")
    if plugins is None:
        plugins = server.get("plugins", [])
    return list(plugins)


def plugin_db(data: dict[str, Any], plugin: str) -> str:
    """Return the database name configured for a plugin, or "" for DB-less plugins."""
    plugins = data.get("plugins", {})
    if plugin not in plugins:
        die(f"plugin '{plugin}' is not declared under 'plugins' in inventory.yml")
    plugin_cfg = plugins.get(plugin) or {}
    return str(plugin_cfg.get("database") or "")


def plugin_settings(data: dict[str, Any], plugin: str) -> dict[str, Any]:
    """Return the settings a plugin's inventory entry overrides in its settings.jsonc."""
    plugin_cfg = data.get("plugins", {}).get(plugin) or {}
    return dict(plugin_cfg.get("settings") or {})


def runtime_image(data: dict[str, Any]) -> str:
    """Return the default server runtime image ref."""
    image = data.get("defaults", {}).get("runtime_image")
    if not image:
        die("no 'defaults.runtime_image' in inventory.yml")
    return str(image)
