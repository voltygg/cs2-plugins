"""Deploy CS2 servers hosted on a Pterodactyl panel through its client API."""

from __future__ import annotations

import io
import json
import os
import re
import tarfile
import time
import urllib.error
import urllib.parse
import urllib.request
import uuid
from contextlib import closing
from typing import Any

from . import inventory, render
from .common import DEPLOY, die, load_server_env, repo_path
from .rcon import RconClient

MMS_BASE = os.environ.get("MMS_BASE", "https://mms.alliedmods.net/mmsdrop/2.0")
# Cloudflare in front of many panels rejects urllib's default agent.
USER_AGENT = "cs2-plugins-deploy"
GAME_CSGO_LINE = re.compile(r"^([ \t]*)Game[ \t]+csgo[ \t]*(\r?)$", re.MULTILINE)
RUNNING_PLUGIN_LINE = re.compile(r"^\s*\[\d+\]\s+(?!<)", re.MULTILINE)
BROKEN_PLUGIN_STATUS = re.compile(r"<(ERROR|FAILED|REFUSED)>", re.IGNORECASE)


class PanelClient:
    """Client API calls for one inventory server."""

    def __init__(self, server: dict[str, Any]) -> None:
        for key in ("panel_url", "panel_server", "host"):
            if not server.get(key):
                die(f"server '{server['id']}' needs `{key}` in inventory.yml")
        api_key = os.environ.get("PANEL_API_KEY")
        if not api_key:
            die(f"PANEL_API_KEY is not set in the '{server['id']}' env")
        panel_url = str(server["panel_url"]).rstrip("/")
        self._base = f"{panel_url}/api/client/servers/{server['panel_server']}"
        self._headers = {
            "Authorization": f"Bearer {api_key}",
            "Accept": "application/json",
            "User-Agent": USER_AGENT,
        }

    def state(self) -> str:
        return json.loads(self._call("GET", "/resources"))["attributes"]["current_state"]

    def power(self, signal: str, wanted_state: str, timeout: float) -> bool:
        """Send a power signal and wait until the server reaches wanted_state."""
        self._call("POST", "/power", {"signal": signal})
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if self.state() == wanted_state:
                return True
            time.sleep(5)
        return False

    def read(self, path: str) -> str:
        """Return a file's text, or "" when it does not exist."""
        return self._call("GET", "/files/contents", query={"file": path}, missing_ok=True).decode()

    def write(self, path: str, text: str) -> None:
        self._call("POST", "/files/write", text.encode(), query={"file": path})

    def delete(self, root: str, names: list[str]) -> list[str]:
        """Delete the entries under root that exist and return them."""
        present = [name for name in names if self._exists(f"{root}/{name}")]
        if present:
            self._call("POST", "/files/delete", {"root": root, "files": present})
        return present

    def extract(self, directory: str, name: str, archive: bytes) -> None:
        """Upload an archive, unpack it into directory, and remove it."""
        signed_url = json.loads(self._call("GET", "/files/upload"))["attributes"]["url"]
        separator = "&" if "?" in signed_url else "?"
        url = f"{signed_url}{separator}{urllib.parse.urlencode({'directory': directory})}"
        boundary = uuid.uuid4().hex
        head = (
            f"--{boundary}\r\n"
            f'Content-Disposition: form-data; name="files"; filename="{name}"\r\n'
            "Content-Type: application/octet-stream\r\n\r\n"
        )
        body = head.encode() + archive + f"\r\n--{boundary}--\r\n".encode()
        headers = {"Content-Type": f"multipart/form-data; boundary={boundary}"}
        request = urllib.request.Request(url, body, {**headers, "User-Agent": USER_AGENT})
        _open(request, f"panel upload of {name}")
        self._call("POST", "/files/decompress", {"root": directory, "file": name})
        self._call("POST", "/files/delete", {"root": directory, "files": [name]})

    def _exists(self, path: str) -> bool:
        directory, _, name = path.rpartition("/")
        listing = self._call("GET", "/files/list", query={"directory": directory}, missing_ok=True)
        entries = json.loads(listing)["data"] if listing else []
        return any(entry["attributes"]["name"] == name for entry in entries)

    def _call(
        self,
        method: str,
        path: str,
        body: dict[str, Any] | bytes | None = None,
        *,
        query: dict[str, str] | None = None,
        missing_ok: bool = False,
    ) -> bytes:
        url = self._base + path + (f"?{urllib.parse.urlencode(query)}" if query else "")
        headers = dict(self._headers)
        if isinstance(body, dict):
            body = json.dumps(body).encode()
            headers["Content-Type"] = "application/json"
        elif body is not None:
            headers["Content-Type"] = "text/plain"
        request = urllib.request.Request(url, body, headers, method=method)
        return _open(request, f"panel {method} {path}", missing_ok=missing_ok)


def deploy_server(server_id: str, package_dir: str, *, dry_run: bool) -> None:
    """Upload plugins, keep Metamod current, and restart one panel server."""
    data = inventory.load()
    server = inventory.find_server(data, server_id)
    load_server_env(server_id, required=True)
    client = PanelClient(server)
    instance = _only_instance(server)
    plugins = inventory.instance_plugins(server, instance)
    game_dir = str(server["game_dir"]).rstrip("/")

    render_dir = DEPLOY / ".render" / server_id
    render.render_addons(data, server, instance, repo_path(package_dir), render_dir)
    archive = io.BytesIO()
    with tarfile.open(fileobj=archive, mode="w:gz") as tar:
        tar.add(render_dir / "addons", arcname="addons")

    gameinfo_path = f"{game_dir}/gameinfo.gi"
    gameinfo = client.read(gameinfo_path)
    if not gameinfo:
        die(f"{gameinfo_path} not found on {server_id}; is CS2 installed and game_dir right?")
    patched_gameinfo = add_metamod_search_path(gameinfo)

    metamod_stamp = f"{game_dir}/addons/metamod/.mms-build"
    installed_metamod = client.read(metamod_stamp).strip()
    metamod = _latest_metamod_url() or installed_metamod
    if not metamod:
        die(f"Metamod is not installed on {server_id} and {MMS_BASE} is unreachable")

    print(f"=== Deploying to {server_id} ({server['panel_url']}, state: {client.state()}) ===")
    print(f"    plugins:  {' '.join(plugins) or '<none>'} ({len(archive.getvalue()) // 1024} KiB)")
    print(f"    metamod:  {metamod}")
    print(f"    gameinfo: {'patch' if patched_gameinfo != gameinfo else 'ok'}")
    if dry_run:
        print("=== Dry run complete; the server was not changed ===")
        return

    _stop(client)
    if metamod != installed_metamod:
        print(f"    installing Metamod: {metamod}")
        client.delete(f"{game_dir}/addons/metamod", ["bin"])
        client.extract(game_dir, "metamod.tar.gz", _download(metamod))
        client.write(metamod_stamp, metamod + "\n")
    client.extract(game_dir, "cs2-plugins.tar.gz", archive.getvalue())
    unassigned = [name for name in inventory.used_plugins(data) if name not in plugins]
    vdfs = [f"metamod/{name}.vdf" for name in unassigned]
    if removed := client.delete(f"{game_dir}/addons", unassigned + vdfs):
        print(f"    removed unassigned plugins: {' '.join(removed)}")
    if patched_gameinfo != gameinfo:
        client.write(gameinfo_path, patched_gameinfo)
    _start(client)
    _verify_plugins(server, instance, len(plugins))
    print(f"=== Deploy to {server_id} complete ===")


def update_server(server_id: str, *, dry_run: bool) -> None:
    """Restart one panel server so its egg runs the SteamCMD update."""
    server = inventory.find_server(inventory.load(), server_id)
    load_server_env(server_id, required=True)
    if dry_run:
        print(f"DRY: restart {server_id} through {server['panel_url']}")
        return
    client = PanelClient(server)
    instance = _only_instance(server)
    _stop(client)
    _start(client)
    _verify_plugins(server, instance, len(inventory.instance_plugins(server, instance)))
    print(f"=== Update for {server_id} complete ===")


def add_metamod_search_path(gameinfo: str) -> str:
    """Return gameinfo.gi with Metamod's search path above the first `Game csgo` line."""
    if "csgo/addons/metamod" in gameinfo:
        return gameinfo
    patched, count = GAME_CSGO_LINE.subn(r"\1Game\tcsgo/addons/metamod\2\n\g<0>", gameinfo, 1)
    if not count:
        die("gameinfo.gi has no `Game csgo` line; its format changed")
    return patched


def _only_instance(server: dict[str, Any]) -> dict[str, Any]:
    if len(server["instances"]) != 1:
        die(f"panel server '{server['id']}' needs exactly one instance")
    return server["instances"][0]


def _stop(client: PanelClient) -> None:
    # Overwriting a loaded .so can crash a running server.
    if client.state() == "offline":
        return
    print("    stopping server")
    if not client.power("stop", "offline", 60) and not client.power("kill", "offline", 30):
        die("server did not stop")


def _start(client: PanelClient) -> None:
    print("    starting server")
    if not client.power("start", "running", 600):
        die("server did not reach `running` within 10 minutes; check the panel console")


def _verify_plugins(
    server: dict[str, Any], instance: dict[str, Any], expected: int, timeout: float = 300
) -> None:
    password = os.environ.get(f"RCON_{instance['name']}")
    if not password:
        print(f"WARNING: RCON_{instance['name']} is not set; skipping the plugin load check")
        return
    host, port = str(server["host"]), int(instance["port"])
    listing = ""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            with closing(RconClient(host, port, password)) as rcon:
                listing = rcon.execute("meta list")
        except OSError:
            listing = ""
        if BROKEN_PLUGIN_STATUS.search(listing):
            die(f"a plugin failed to load:\n{listing}")
        if len(RUNNING_PLUGIN_LINE.findall(listing)) >= expected:
            print(listing)
            return
        time.sleep(10)
    die(
        f"{expected} plugins did not load within {int(timeout)}s (RCON {host}:{port}); "
        f"a CS2 update may have reset gameinfo.gi. Last `meta list`:\n{listing or '<none>'}"
    )


def _latest_metamod_url() -> str:
    """Return the newest Linux Metamod build URL, or "" when the mirror is unreachable."""
    request = urllib.request.Request(
        f"{MMS_BASE}/mmsource-latest-linux", headers={"User-Agent": USER_AGENT}
    )
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            name = response.read().decode().strip()
    except OSError:
        return ""
    return f"{MMS_BASE}/{name}" if name else ""


def _download(url: str) -> bytes:
    request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    return _open(request, f"download of {url}")


def _open(request: urllib.request.Request, label: str, *, missing_ok: bool = False) -> bytes:
    try:
        with urllib.request.urlopen(request, timeout=300) as response:
            return response.read()
    except urllib.error.HTTPError as exc:
        if missing_ok and exc.code == 404:
            return b""
        die(f"{label} returned HTTP {exc.code}: {exc.read().decode(errors='replace')[:300]}")
    except urllib.error.URLError as exc:
        die(f"{label} failed: {exc.reason}")
