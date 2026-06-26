#!/usr/bin/env python3
"""Render Docker Compose deployment artifacts for one server.

Inputs:
  * deploy/inventory.yml for servers/plugins/database defaults.
  * exported environment variables loaded from secrets/servers/<id>/.sops.env.
  * package/<plugin>/addons bundles produced by deploy/scripts/package-plugin.sh.

Outputs under deploy/.render/<server>/:
  * docker-compose.yml
  * bundles/addons/... merged plugin tree with rendered settings.jsonc
  * instances/<name>/server.env and instances/<name>/cs2/pre.sh
"""

import argparse
import json
import os
import re
import shutil
import stat
import sys
from pathlib import Path
from typing import Any

import inventory
import yaml

ROOT = Path(__file__).resolve().parents[2]
DEPLOY = ROOT / "deploy"


def die(message: str) -> None:
    raise SystemExit(f"ERROR: {message}")


def json_string_content(value: str) -> str:
    return json.dumps(value)[1:-1]


def strip_jsonc(raw: str) -> str:
    return re.sub(r"(?m)^\s*//.*$", "", raw)


def render_settings(data: dict[str, Any], server_id: str, plugin: str, out: Path) -> None:
    db = data.get("database", {})
    env = {
        "DB_HOST": str(db.get("host", "")),
        "DB_PORT": str(db.get("port", "")),
        "DB_NAME": str(inventory.plugin_db(data, plugin)),
        "DB_USER": str(db.get("user", "")),
        "DB_PASSWORD": os.environ.get("DB_PASSWORD", ""),
        "DB_SSLMODE": str(db.get("sslMode", "prefer")),
        "CHEAT_ROOM_URL": os.environ.get("CHEAT_ROOM_URL", ""),
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
    source_addons = package_dir / plugin / "addons"
    if not source_addons.is_dir():
        die(f"no bundle at {source_addons}; run deploy/scripts/package-plugin.sh {plugin}")
    target_addons = out_dir / "bundles" / "addons"
    shutil.copytree(source_addons, target_addons, dirs_exist_ok=True)
    render_settings(data, server_id, plugin, target_addons / plugin / "configs" / "settings.jsonc")


def dotenv_value(value: str) -> str:
    return json.dumps(value)


def write_env_file(instance: dict[str, Any], out: Path) -> None:
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


PRE_SH = """#!/usr/bin/env bash
set -euo pipefail

Root="/home/steam/cs2-dedicated"
Csgo="$Root/game/csgo"
AddonsSrc="/home/steam/plugin-bundles/addons"
MmsBase="${MMS_BASE:-https://mms.alliedmods.net/mmsdrop/2.0}"

if [[ ! -d "$Csgo" ]]; then
    echo "CS2 game directory is not present yet: $Csgo" >&2
    exit 0
fi

if [[ -d "$AddonsSrc" ]]; then
    mkdir -p "$Csgo/addons"
    cp -a "$AddonsSrc/." "$Csgo/addons/"
fi

if [[ -f "$Csgo/gameinfo.gi" ]] && ! grep -q 'csgo/addons/metamod' "$Csgo/gameinfo.gi"; then
    awk '
        !done && $0 ~ /[[:space:]]Game[[:space:]]+csgo[[:space:]]*$/ {
            match($0, /^[[:space:]]*/); indent = substr($0, 1, RLENGTH)
            printf "%sGame\\tcsgo/addons/metamod\\n", indent
            done = 1
        }
        { print }
    ' "$Csgo/gameinfo.gi" > "$Csgo/gameinfo.gi.tmp"
    mv "$Csgo/gameinfo.gi.tmp" "$Csgo/gameinfo.gi"
fi

if [[ ! -d "$Csgo/addons/metamod/bin" ]]; then
    if [[ -z "${MMS_URL:-}" ]]; then
        latest="$(curl -fsSL "$MmsBase/mmsource-latest-linux")"
        MMS_URL="$MmsBase/$latest"
    fi
    tmp="$(mktemp -d)"
    curl -fsSL "$MMS_URL" -o "$tmp/mms.tar.gz"
    tar -xzf "$tmp/mms.tar.gz" -C "$Csgo"
    rm -rf "$tmp"
fi
"""


def write_pre_hook(out: Path) -> None:
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(PRE_SH, encoding="utf-8", newline="\n")
    out.chmod(out.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)


def compose_for(server: dict[str, Any], runtime_image: str) -> dict[str, Any]:
    services: dict[str, Any] = {}
    for instance in server["instances"]:
        name = str(instance["name"])
        port = str(instance["port"])
        service = f"cs2-{name}"
        services[service] = {
            "image": runtime_image,
            "container_name": f"{server['id']}-{service}",
            "restart": "unless-stopped",
            "env_file": [f"./instances/{name}/server.env"],
            "ports": [f"{port}:{port}/udp", f"{port}:{port}/tcp"],
            "volumes": [
                f"./instances/{name}/cs2:/home/steam/cs2-dedicated",
                "./bundles:/home/steam/plugin-bundles:ro",
            ],
        }
    return {"services": services}


def render(server_id: str, package_dir: Path, out_dir: Path, runtime_image: str | None) -> None:
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
        write_env_file(instance, instance_dir / "server.env")
        write_pre_hook(instance_dir / "cs2" / "pre.sh")

    compose = compose_for(server, image)
    (out_dir / "docker-compose.yml").write_text(
        yaml.safe_dump(compose, sort_keys=False), encoding="utf-8", newline="\n"
    )
    print(f"rendered {server_id} -> {out_dir}")


def main(argv: list[str]) -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--server", required=True)
    parser.add_argument("--package-dir", default="package")
    parser.add_argument("--out-dir")
    parser.add_argument("--runtime-image")
    args = parser.parse_args(argv)

    out = Path(args.out_dir) if args.out_dir else DEPLOY / ".render" / args.server
    render(args.server, Path(args.package_dir), out, args.runtime_image)


if __name__ == "__main__":
    main(sys.argv[1:])
