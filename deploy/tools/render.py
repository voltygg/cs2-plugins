"""Render plugin bundles for every server and Compose trees for Docker hosts."""

from __future__ import annotations

import json
import os
import re
import shutil
import textwrap
from pathlib import Path
from typing import Any

import yaml

from . import inventory
from .common import DEPLOY, ROOT, die

PRE_HOOK_TEMPLATE = DEPLOY / "templates" / "pre.sh"
COMPOSE_SERVICE_TEMPLATE = DEPLOY / "templates" / "compose.service.yml"
PLACEHOLDER = re.compile(r"\$\{([A-Za-z_][A-Za-z0-9_]*)\}")
COMMENT_OR_STRING = re.compile(r'"(?:\\.|[^"\\])*"|//[^\n]*|/\*.*?\*/', re.DOTALL)


def read_jsonc(path: Path) -> Any:
    """Parse a JSONC file, keeping `//` inside strings such as URLs."""
    text = COMMENT_OR_STRING.sub(
        lambda match: match.group(0) if match.group(0).startswith('"') else "",
        path.read_text(encoding="utf-8"),
    )
    return json.loads(text)


def fill_placeholders(text: str, variables: dict[str, str], source: str) -> str:
    """Replace every ${NAME} in text; a name missing from variables is an error."""

    def lookup(match: re.Match[str]) -> str:
        name = match.group(1)
        if name not in variables:
            die(f"${{{name}}} in {source} is not set")
        return variables[name]

    return PLACEHOLDER.sub(lookup, text)


def render_addons(
    data: dict[str, Any],
    server: dict[str, Any],
    instance: dict[str, Any],
    package_dir: Path,
    bundles_dir: Path,
) -> None:
    """Rebuild one instance's `addons` tree under bundles_dir with rendered settings."""
    shutil.rmtree(bundles_dir, ignore_errors=True)
    addons = bundles_dir / "addons"
    addons.mkdir(parents=True)
    for plugin in inventory.instance_plugins(server, instance):
        bundle = package_dir / plugin / "addons"
        if not bundle.is_dir():
            die(f"no bundle at {bundle}; run: python -m deploy.tools.cli package {plugin}")
        shutil.copytree(bundle, addons, dirs_exist_ok=True)
        settings = render_settings(data, str(server["id"]), instance, plugin)
        settings_path = addons / plugin / "configs" / "settings.jsonc"
        settings_path.parent.mkdir(parents=True, exist_ok=True)
        rendered = json.dumps(settings, indent=2, ensure_ascii=False) + "\n"
        settings_path.write_text(rendered, encoding="utf-8", newline="\n")


def render_settings(
    data: dict[str, Any], server_id: str, instance: dict[str, Any], plugin: str
) -> dict[str, Any]:
    """Return a plugin's own settings.jsonc with inventory overrides and its database."""
    configs = ROOT / "plugins" / plugin / "configs"
    if not (configs / "settings.jsonc").is_file():
        die(f"no plugin settings at {configs / 'settings.jsonc'}")
    settings = read_jsonc(configs / "settings.jsonc")
    schema_path = configs / "settings.schema.json"
    schema = json.loads(schema_path.read_text(encoding="utf-8")) if schema_path.is_file() else {}

    name = str(instance["name"])
    variables = {
        **os.environ,
        # Per-server admin grants reference the tag, so it must stay stable.
        "SERVER_TAG": f"{server_id}-{name}",
        "SERVER_NAME": str(instance.get("hostname", f"CS2 {name}")),
    }
    json_escaped = {key: json.dumps(value)[1:-1] for key, value in variables.items()}
    overrides_text = json.dumps(inventory.plugin_settings(data, plugin))
    overrides = json.loads(fill_placeholders(overrides_text, json_escaped, f"{plugin} settings"))
    merge_settings(plugin, settings, overrides, schema)

    db_name = inventory.plugin_db(data, plugin)
    if db_name:
        settings["database"] = {
            **settings.get("database", {}),
            **database_settings(data, db_name, f"{server_id}/{plugin}"),
        }
    return settings


def merge_settings(
    plugin: str,
    settings: dict[str, Any],
    overrides: dict[str, Any],
    schema: dict[str, Any],
    prefix: str = "",
) -> None:
    """Deep-merge overrides into settings; lists and scalars replace.

    A key the schema does not define fails, because the plugin would silently ignore it.
    """
    known = schema.get("properties")
    for key, value in overrides.items():
        if known is not None and key not in known:
            die(f"inventory sets unknown {plugin} setting '{prefix}{key}'")
        if not isinstance(value, dict):
            settings[key] = value
            continue
        if not isinstance(settings.get(key), dict):
            settings[key] = {}
        merge_settings(plugin, settings[key], value, (known or {}).get(key, {}), f"{prefix}{key}.")


def database_settings(data: dict[str, Any], db_name: str, target: str) -> dict[str, Any]:
    """Return the framework DatabaseConfig fields for one plugin database."""
    db = data.get("database", {})
    host = os.environ.get("DB_HOST") or db.get("host")
    password = os.environ.get("DB_PASSWORD")
    required = {
        "DB_HOST": host,
        "database.port": db.get("port"),
        "database.user": db.get("user"),
        "DB_PASSWORD": password,
    }
    for name, value in required.items():
        if not value:
            die(f"{name} is not set for {target}")
    return {
        "host": host,
        "port": int(db["port"]),
        "database": db_name,
        "username": db["user"],
        "password": password,
        "sslMode": db.get("sslMode", "prefer"),
    }


def render(server_id: str, package_dir: Path, out_dir: Path, runtime_image: str | None) -> None:
    """Render the Compose tree for one Docker inventory server."""
    data = inventory.load()
    server = inventory.find_server(data, server_id)
    image = runtime_image or os.environ.get("RUNTIME_IMAGE") or server.get("runtime_image")
    if not image:
        die(f"no runtime image configured for {server_id}")
    if not server["instances"]:
        die(f"server '{server_id}' has no instances")

    for instance in server["instances"]:
        instance_dir = out_dir / "instances" / str(instance["name"])
        render_addons(data, server, instance, package_dir, instance_dir / "bundles")
        write_env_file(instance, instance_dir / ".env")
        pre_hook = instance_dir / "pre.sh"
        pre_hook.write_text(
            PRE_HOOK_TEMPLATE.read_text(encoding="utf-8"), encoding="utf-8", newline="\n"
        )
        pre_hook.chmod(0o755)

    compose = render_compose(server, str(image))
    (out_dir / "docker-compose.yml").write_text(compose, encoding="utf-8", newline="\n")
    print(f"rendered {server_id} -> {out_dir}")


def write_env_file(instance: dict[str, Any], out: Path) -> None:
    """Write one CS2 instance env_file consumed by Docker Compose."""
    name = str(instance["name"])
    token = os.environ.get(f"GSLT_{name}", "")
    if not token:
        print(f"WARNING: GSLT_{name} is not set; instance '{name}' will start in LAN mode")
    env = {
        "SRCDS_TOKEN": token,
        "CS2_RCONPW": os.environ.get(f"RCON_{name}", ""),
        "CS2_PORT": str(instance["port"]),
        "CS2_STARTMAP": str(instance.get("map", "de_dust2")),
        "CS2_SERVERNAME": os.environ.get(
            f"CS2_HOSTNAME_{name}", str(instance.get("hostname", f"CS2 {name}"))
        ),
    }
    # Do not add +exec server.cfg here; that can re-enable VAC after -insecure.
    if instance.get("insecure"):
        env["CS2_ADDITIONAL_ARGS"] = "-insecure"
    lines = "".join(f"{key}={json.dumps(value)}\n" for key, value in env.items())
    out.write_text(lines, encoding="utf-8", newline="\n")


def render_compose(server: dict[str, Any], runtime_image: str) -> str:
    """Render the Docker Compose file for a resolved server from the service template."""
    template = COMPOSE_SERVICE_TEMPLATE.read_text(encoding="utf-8")
    services = ""
    for instance in server["instances"]:
        name = str(instance["name"])
        variables = {
            "INSTANCE_NAME": name,
            "CONTAINER_NAME": f"{server['id']}-cs2-{name}",
            "RUNTIME_IMAGE": runtime_image,
            "PORT": str(instance["port"]),
            "CS2_SERVER_DIR": f"{str(server['cs2_root']).rstrip('/')}/server",
        }
        service = fill_placeholders(template, variables, COMPOSE_SERVICE_TEMPLATE.name)
        services += textwrap.indent(service, "  ")

    document = f"name: cs2\nservices:\n{services}"
    yaml.safe_load(document)
    return document
