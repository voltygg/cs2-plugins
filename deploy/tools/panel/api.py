"""The panel's client API."""

import time
from typing import Any

import httpx

from deploy.tools.config.servers import PanelServer
from deploy.tools.errors import DeployError

# Cloudflare in front of many panels rejects default client agents.
USER_AGENT = "cs2-plugins-deploy"

# For requests that carry no panel key: the node's upload URL and the Metamod mirror.
web = httpx.Client(headers={"User-Agent": USER_AGENT}, timeout=300, follow_redirects=True)


class PanelApi:
    """The panel's client API for one server."""

    def __init__(self, server: PanelServer, api_key: str) -> None:
        self._http = httpx.Client(
            base_url=server.api_url,
            headers={
                "Authorization": f"Bearer {api_key}",
                "Accept": "application/json",
                "User-Agent": USER_AGENT,
            },
            timeout=300,
        )

    def state(self) -> str:
        return self._request("GET", "/resources").json()["attributes"]["current_state"]

    def power(self, signal: str, wanted_state: str, timeout: float) -> bool:
        """Send a power signal and wait until the server reaches wanted_state."""
        self._request("POST", "/power", json={"signal": signal})
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if self.state() == wanted_state:
                return True
            time.sleep(5)
        return False

    def read(self, path: str) -> str:
        """A file's text, or "" when it does not exist."""
        if self._entry(path) is None:
            return ""
        response = self._request_if_found("GET", "/files/contents", params={"file": path})
        return response.text if response else ""

    def write(self, path: str, text: str) -> None:
        self._request(
            "POST",
            "/files/write",
            params={"file": path},
            content=text.encode(),
            headers={"Content-Type": "text/plain"},
        )

    def delete(self, root: str, names: list[str]) -> list[str]:
        """Delete the entries under root that exist and return them."""
        present = [name for name in names if self._entry(f"{root}/{name}") is not None]
        if present:
            self._request("POST", "/files/delete", json={"root": root, "files": present})
        return present

    def extract(self, directory: str, name: str, archive: bytes) -> None:
        """Upload an archive, unpack it into directory, and remove it."""
        upload_url = self._request("GET", "/files/upload").json()["attributes"]["url"]
        # The signed URL goes to the node, which needs no panel key. Merge into its query: a
        # params= argument would replace it, dropping the token the node authenticates with.
        target = httpx.URL(upload_url).copy_merge_params({"directory": directory})
        try:
            response = web.post(
                target,
                files={"files": (name, archive, "application/octet-stream")},
            )
        except httpx.TransportError as error:
            raise DeployError(f"upload of {name} failed: {error}") from None
        self._check(response, f"upload of {name}")
        self._request("POST", "/files/decompress", json={"root": directory, "file": name})
        self._request("POST", "/files/delete", json={"root": directory, "files": [name]})

    def is_link(self, path: str) -> bool:
        entry = self._entry(path)
        return bool(entry and entry["is_symlink"])

    def _entry(self, path: str) -> dict[str, Any] | None:
        directory, _, name = path.rpartition("/")
        # Some nodes answer a missing path with 500 instead of 404, so confirm the parent first.
        if directory and self._entry(directory) is None:
            return None
        listing = {"directory": directory or "/"}
        response = self._request_if_found("GET", "/files/list", params=listing)
        entries = response.json()["data"] if response else []
        return next(
            (entry["attributes"] for entry in entries if entry["attributes"]["name"] == name), None
        )

    def _request(self, method: str, path: str, **options: Any) -> httpx.Response:
        response = self._send(method, path, **options)
        self._check(response, f"panel {method} {path}")
        return response

    def _request_if_found(self, method: str, path: str, **options: Any) -> httpx.Response | None:
        response = self._send(method, path, **options)
        if response.status_code == 404:
            return None
        self._check(response, f"panel {method} {path}")
        return response

    def _send(self, method: str, path: str, **options: Any) -> httpx.Response:
        try:
            return self._http.request(method, path, **options)
        except httpx.TransportError as error:
            raise DeployError(f"panel {method} {path} failed: {error}") from None

    @staticmethod
    def _check(response: httpx.Response, label: str) -> None:
        if response.is_error:
            detail = response.text[:300]
            raise DeployError(f"{label} returned HTTP {response.status_code}: {detail}")
