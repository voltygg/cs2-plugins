#!/usr/bin/env python3
"""Render Docker Compose deployment artifacts for one server.

Inputs:
  * deploy/inventory.yml for servers/plugins/database defaults.
  * exported environment variables loaded from secrets/servers/<id>/.env.
  * package/<plugin>/addons bundles produced by deploy/tools/cli.py package.

Outputs under deploy/.render/<server>/:
  * docker-compose.yml
  * bundles/addons/... merged plugin tree with rendered settings.jsonc
  * instances/<name>/.env and instances/<name>/pre.sh
"""

from __future__ import annotations

import json
import os
import re
import shutil
import stat
from pathlib import Path
from typing import Any

import inventory
import yaml
from common import DEPLOY, die

PRE_HOOK_TEMPLATE = DEPLOY / "templates" / "pre.sh"


def json_string_content(value: str) -> str:
    """Return JSON-escaped string content without surrounding quotes."""
    return json.dumps(value)[1:-1]


def strip_jsonc(raw: str) -> str:
    """Remove whole-line JSONC comments before JSON validation."""
    return re.sub(r"(?m)^\s*//.*$", "", raw)


def render_settings(data: dict[str, Any], server_id: str, plugin: str, out: Path) -> None:
    """Render one plugin settings.jsonc from inventory and environment."""
    db = data.get("database", {})
    env = {
        "DB_HOST": str(db.get("host", "")),
        "DB_PORT": str(db.get("port", "")),
        "DB_NAME": str(inventory.plugin_db(data, plugin)),
        "DB_USER": str(db.get("user", "")),
        "DB_PASSWORD": os.environ.get("DB_PASSWORD", ""),
        "DB_SSLMODE": str(db.get("sslMode", "prefer")),
        "CHEAT_API_KEY": os.environ.get("CHEAT_API_KEY", ""),
    }
    for key in ("DB_HOST", "DB_PORT", "DB_NAME", "DB_USER", "DB_PASSWORD", "DB_SSLMODE"):
        if not env[key]:
            die(f"required var {key} is empty for {server_id}/{plugin}")

    template = DEPLOY / "templates" / "plugins" / plugin / "settings.jsonc"
    if not template.is_file():
        die(f"no settings template at {template}")
    rendered = template.read_text(encoding="utf-8")
    for key, value in env.items():
        replacement = value if key == "DB_PORT" else json_string_content(value)
        rendered = rendered.replace("${" + key + "}", replacement)

    body = strip_jsonc(rendered)
    leftover = sorted(set(re.findall(r"\$\{[A-Za-z_][A-Za-z0-9_]*\}", body)))
    if leftover:
        die("unsubstituted placeholders: " + ", ".join(leftover))
    json.loads(body)

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(rendered, encoding="utf-8", newline="\n")


def copy_plugin_bundle(
    data: dict[str, Any], server_id: str, plugin: str, package_dir: Path, out_dir: Path
) -> None:
    """Copy one packaged plugin bundle and render its server config."""
    source_addons = package_dir / plugin / "addons"
    if not source_addons.is_dir():
        die(f"no bundle at {source_addons}; run deploy/tools/cli.py package {plugin}")
    target_addons = out_dir / "bundles" / "addons"
    shutil.copytree(source_addons, target_addons, dirs_exist_ok=True)
    render_settings(data, server_id, plugin, target_addons / plugin / "configs" / "settings.jsonc")


def dotenv_value(value: str) -> str:
    """Encode a value for a dotenv file."""
    return json.dumps(value)


def write_env_file(instance: dict[str, Any], out: Path) -> None:
    """Write one CS2 instance env_file consumed by Docker Compose."""
    name = str(instance["name"])
    port = str(instance["port"])
    env = {
        "SRCDS_TOKEN": os.environ.get(f"GSLT_{name}", ""),
        "CS2_RCONPW": os.environ.get(f"RCON_{name}", ""),
        "CS2_PORT": port,
        "CS2_STARTMAP": str(instance.get("map", "de_dust2")),
        "CS2_SERVERNAME": os.environ.get(
            f"CS2_HOSTNAME_{name}", str(instance.get("hostname", f"CS2 {name}"))
        ),
    }
    out.parent.mkdir(parents=True, exist_ok=True)
    lines = [f"{key}={dotenv_value(value)}" for key, value in env.items()]
    out.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def write_pre_hook(out: Path) -> None:
    """Write the CS2 container pre-launch hook and mark it executable."""
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(PRE_HOOK_TEMPLATE.read_text(encoding="utf-8"), encoding="utf-8", newline="\n")
    out.chmod(out.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)


def compose_for(server: dict[str, Any], runtime_image: str) -> dict[str, Any]:
    """Build the Docker Compose model for a resolved server."""
    services: dict[str, Any] = {}
    cs2_server_dir = f"{str(server['cs2_root']).rstrip('/')}/server"
    for instance in server["instances"]:
        name = str(instance["name"])
        port = str(instance["port"])
        service = f"cs2-{name}"
        services[service] = {
            "image": runtime_image,
            "container_name": f"{server['id']}-{service}",
            "restart": "unless-stopped",
            "env_file": [f"./instances/{name}/.env"],
            "ports": [f"{port}:{port}/udp", f"{port}:{port}/tcp"],
            # reach host Postgres from the bridge network
            "extra_hosts": ["host.docker.internal:host-gateway"],
            "volumes": [
                f"{cs2_server_dir}:/home/steam/cs2-dedicated",
                f"./instances/{name}/pre.sh:/home/steam/cs2-dedicated/pre.sh:ro",
                "./bundles:/home/steam/plugin-bundles:ro",
            ],
        }
    return {"services": services}


def render(server_id: str, package_dir: Path, out_dir: Path, runtime_image: str | None) -> None:
    """Render all deploy artifacts for one inventory server."""
    data = inventory.load()
    server = inventory.find_server(data, server_id)
    image = runtime_image or os.environ.get("RUNTIME_IMAGE") or str(server.get("runtime_image", ""))
    if not image:
        die(f"no runtime image configured for {server_id}")
    if not server.get("instances"):
        die(f"server '{server_id}' has no instances")

    if (out_dir / "bundles").exists():
        shutil.rmtree(out_dir / "bundles")
    (out_dir / "bundles").mkdir(parents=True, exist_ok=True)

    for plugin in server["plugins"]:
        copy_plugin_bundle(data, server_id, str(plugin), package_dir, out_dir)

    for instance in server["instances"]:
        if not instance.get("name") or not instance.get("port"):
            die(f"server '{server_id}' has an instance without name/port")
        instance_dir = out_dir / "instances" / str(instance["name"])
        write_env_file(instance, instance_dir / ".env")
        write_pre_hook(instance_dir / "pre.sh")

    compose = compose_for(server, image)
    (out_dir / "docker-compose.yml").write_text(
        yaml.safe_dump(compose, sort_keys=False), encoding="utf-8", newline="\n"
    )
    print(f"rendered {server_id} -> {out_dir}")
