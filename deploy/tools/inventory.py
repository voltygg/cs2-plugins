#!/usr/bin/env python3
"""Query deploy/inventory.yml for the Docker deploy tooling and CI.

The inventory is the single source of truth for which servers run which plugins.
This script resolves `defaults` into each server and emits the pieces the rest of
the pipeline needs:

  servers-json        JSON array of fully-resolved servers (the deploy matrix)
  plugins-json        JSON array of declared plugin names (the build set)
  server   <id>       JSON object for one resolved server
  server-env <id>     `export SRV_*=...` lines (host, ssh, deploy root, plugins, instances)
  plugin-dbs          "<plugin> <dbname>" lines for deploy/scripts/ensure-databases.sh
  db-conn             `export DB_*=...` lines for the shared connection (no plugin)

Requires PyYAML (declared in pyproject.toml; `pip install pyyaml` in CI).
"""

import json
import os
import sys

import yaml

_HERE = os.path.dirname(os.path.abspath(__file__))
INVENTORY_PATH = os.environ.get(
    "INVENTORY_PATH", os.path.normpath(os.path.join(_HERE, "..", "inventory.yml"))
)

# Server fields that fall back to `defaults` when not set on the server itself.
_INHERITED = ("ssh_user", "ssh_port", "deploy_root", "runtime_image")


def load() -> dict:
    with open(INVENTORY_PATH, encoding="utf-8") as fh:
        data = yaml.safe_load(fh) or {}
    if "servers" not in data:
        sys.exit(f"ERROR: no 'servers' in {INVENTORY_PATH}")
    return data


def plugin_db(data: dict, plugin: str) -> str:
    reg = data.get("plugins", {})
    if plugin not in reg:
        sys.exit(f"ERROR: plugin '{plugin}' is not declared under 'plugins' in inventory.yml")
    db = reg[plugin].get("database")
    if not db:
        sys.exit(f"ERROR: plugin '{plugin}' has no 'database' in inventory.yml")
    return db


def resolve_server(data: dict, server: dict) -> dict:
    defaults = data.get("defaults", {})
    out = {"id": server.get("id"), "host": server.get("host")}
    for key in _INHERITED:
        out[key] = server.get(key, defaults.get(key))
    out["environment"] = server.get("environment")
    out["enabled"] = bool(server.get("enabled", True))
    out["plugins"] = list(server.get("plugins", []))
    out["instances"] = list(server.get("instances", []))
    return out


def find_server(data: dict, server_id: str) -> dict:
    for server in data.get("servers", []):
        if server.get("id") == server_id:
            return resolve_server(data, server)
    sys.exit(f"ERROR: server '{server_id}' not found in inventory.yml")


def used_plugins(data: dict) -> list[str]:
    # Build every declared plugin. The committed inventory intentionally has no
    # active servers, but CI should still produce bundles for the plugin registry.
    return list(data.get("plugins", {}).keys())


def cmd_servers_json(data: dict) -> None:
    print(json.dumps([resolve_server(data, s) for s in data["servers"] if s.get("enabled", True)]))


def cmd_plugins_json(data: dict) -> None:
    print(json.dumps(used_plugins(data)))


def cmd_server(data: dict, server_id: str) -> None:
    print(json.dumps(find_server(data, server_id)))


def cmd_plugin_dbs(data: dict) -> None:
    for plugin in used_plugins(data):
        print(f"{plugin} {plugin_db(data, plugin)}")


def cmd_server_env(data: dict, server_id: str) -> None:
    s = find_server(data, server_id)
    instances = " ".join(
        f"{i.get('name')}:{i.get('port', '')}:{i.get('map', '')}" for i in s["instances"]
    )
    _print_exports(
        {
            "SRV_ID": s["id"],
            "SRV_HOST": s["host"],
            "SRV_SSH_USER": s["ssh_user"],
            "SRV_SSH_PORT": s["ssh_port"],
            "SRV_DEPLOY_ROOT": s["deploy_root"],
            "SRV_PLUGINS": " ".join(s["plugins"]),
            "SRV_INSTANCES": instances,
        }
    )


def _conn_env(data: dict) -> dict:
    db = data.get("database", {})
    return {
        "DB_HOST": db.get("host", ""),
        "DB_PORT": db.get("port", ""),
        "DB_USER": db.get("user", ""),
        "DB_SSLMODE": db.get("sslMode", "prefer"),
    }


def _print_exports(env: dict) -> None:
    for key, value in env.items():
        print(f"export {key}={_sh_quote(str(value))}")


def cmd_db_conn(data: dict) -> None:
    _print_exports(_conn_env(data))


def _sh_quote(value: str) -> str:
    return "'" + value.replace("'", "'\\''") + "'"


def main(argv: list[str]) -> None:
    if not argv:
        sys.exit(__doc__)
    data = load()
    cmd, rest = argv[0], argv[1:]
    if cmd == "servers-json":
        cmd_servers_json(data)
    elif cmd == "plugins-json":
        cmd_plugins_json(data)
    elif cmd == "server" and len(rest) == 1:
        cmd_server(data, rest[0])
    elif cmd == "server-env" and len(rest) == 1:
        cmd_server_env(data, rest[0])
    elif cmd == "plugin-dbs":
        cmd_plugin_dbs(data)
    elif cmd == "db-conn":
        cmd_db_conn(data)
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main(sys.argv[1:])
