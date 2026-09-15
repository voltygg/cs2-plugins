"""Metamod:Source on a panel server."""

from typing import Self

import httpx

from deploy.tools.errors import DeployError
from deploy.tools.panel.api import PanelApi, web


class Metamod:
    """A panel server's Metamod install, kept at the mirror's newest Linux build."""

    MIRROR = "https://mms.alliedmods.net/mmsdrop/2.0"
    # Relative to game_dir; holds the URL of the installed build.
    STAMP = "addons/metamod/.mms-build"

    def __init__(self, api: PanelApi, game_dir: str, installed: str, wanted: str) -> None:
        self._api = api
        self._game_dir = game_dir
        self.installed = installed
        self.wanted = wanted
        self._archive = b""

    @classmethod
    def read(cls, api: PanelApi, game_dir: str) -> Self:
        installed = api.read(f"{game_dir}/{cls.STAMP}").strip()
        wanted = cls.latest_url() or installed
        if not wanted:
            raise DeployError(f"Metamod is not installed and {cls.MIRROR} is unreachable")
        return cls(api, game_dir, installed, wanted)

    @classmethod
    def latest_url(cls) -> str:
        """The newest build's URL, or "" when the mirror is unreachable."""
        try:
            response = web.get(f"{cls.MIRROR}/mmsource-latest-linux", timeout=30)
            response.raise_for_status()
        except httpx.HTTPError:
            return ""
        name = response.text.strip()
        return f"{cls.MIRROR}/{name}" if name else ""

    @property
    def outdated(self) -> bool:
        return self.wanted != self.installed

    def download(self) -> None:
        """Fetch the wanted build while the server runs, so a mirror failure cannot stop it."""
        if not self.outdated:
            return
        try:
            response = web.get(self.wanted)
            response.raise_for_status()
        except httpx.HTTPError as error:
            raise DeployError(f"download of {self.wanted} failed: {error}") from None
        self._archive = response.content

    def install(self) -> None:
        """Replace the installed build with the downloaded one; the server must be stopped."""
        if not self._archive:
            return
        print(f"    installing Metamod {self.wanted}")
        self._api.delete(f"{self._game_dir}/addons/metamod", ["bin"])
        self._api.extract(self._game_dir, "metamod.tar.gz", self._archive)
        self._api.write(f"{self._game_dir}/{self.STAMP}", self.wanted + "\n")
