from typing import Self

import httpx

from deploy.tools import console
from deploy.tools.errors import DeployError
from deploy.tools.panel.api import PanelApi, web_client


class Metamod:
    """A panel server's Metamod install, kept at the mirror's newest Linux build."""

    MIRROR = "https://mms.alliedmods.net/mmsdrop/2.0"
    # Relative to game_dir; holds the URL of the installed build.
    INSTALLED_BUILD_FILE = "addons/metamod/.mms-build"

    def __init__(self, api: PanelApi, game_dir: str, installed_url: str, latest_url: str) -> None:
        self._api = api
        self._game_dir = game_dir
        self.installed_url = installed_url
        self.latest_url = latest_url
        self._archive = b""

    @classmethod
    def read(cls, api: PanelApi, game_dir: str) -> Self:
        installed = api.read(f"{game_dir}/{cls.INSTALLED_BUILD_FILE}").strip()
        latest = cls.mirror_latest() or installed
        if not latest:
            raise DeployError(f"Metamod is not installed and {cls.MIRROR} is unreachable")
        return cls(api, game_dir, installed, latest)

    @classmethod
    def mirror_latest(cls) -> str:
        """The newest build's URL, or "" when the mirror is unreachable."""
        try:
            response = web_client().get(f"{cls.MIRROR}/mmsource-latest-linux", timeout=30)
            response.raise_for_status()
        except httpx.HTTPError:
            return ""
        name = response.text.strip()
        return f"{cls.MIRROR}/{name}" if name else ""

    @property
    def outdated(self) -> bool:
        return self.latest_url != self.installed_url

    def download(self) -> None:
        """Fetch the latest build while the server runs, so a mirror failure cannot stop it."""
        if not self.outdated:
            return
        try:
            response = web_client().get(self.latest_url)
            response.raise_for_status()
        except httpx.HTTPError as error:
            raise DeployError(f"download of {self.latest_url} failed: {error}") from None
        self._archive = response.content

    def install(self) -> None:
        """Replace the installed build with the downloaded one; the server must be stopped."""
        if not self._archive:
            return
        console.item(f"installing Metamod {self.latest_url}")
        self._api.delete(f"{self._game_dir}/addons/metamod", ["bin"])
        self._api.extract(self._game_dir, "metamod.tar.gz", self._archive)
        self._api.write(f"{self._game_dir}/{self.INSTALLED_BUILD_FILE}", self.latest_url + "\n")
