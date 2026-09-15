"""gameinfo.gi on a panel server."""

import re
from typing import Self

from deploy.tools.errors import DeployError
from deploy.tools.panel.api import PanelApi
from deploy.tools.paths import DEPLOY


class GameInfo:
    """A panel server's csgo/gameinfo.gi, which must list Metamod's search path."""

    SEARCH_PATH = "csgo/addons/metamod"
    GAME_CSGO_LINE = re.compile(r"^([ \t]*)Game[ \t]+csgo[ \t]*(\r?)$", re.MULTILINE)
    # Valve's file from an up-to-date dedicated server; refresh it when a CS2 update changes it.
    TEMPLATE = DEPLOY / "panel" / "gameinfo.gi"

    def __init__(self, api: PanelApi, game_dir: str, current: str, linked_install: bool) -> None:
        self._api = api
        self._game_dir = game_dir
        self.current = current
        self.linked_install = linked_install
        source = self.TEMPLATE.read_bytes().decode() if linked_install else current
        self.patched = self.with_metamod(source)

    @classmethod
    def read(cls, api: PanelApi, game_dir: str) -> Self:
        # Hosts that link every server to one shared CS2 install refuse to read those links.
        if api.is_link(f"{game_dir}/steam.inf"):
            return cls(api, game_dir, "", linked_install=True)
        current = api.read(f"{game_dir}/gameinfo.gi")
        if not current:
            raise DeployError(f"{game_dir}/gameinfo.gi not found; is CS2 installed there?")
        return cls(api, game_dir, current, linked_install=False)

    @classmethod
    def with_metamod(cls, text: str) -> str:
        """text with Metamod's search path above the first `Game csgo` line."""
        if cls.SEARCH_PATH in text:
            return text
        replacement = rf"\1Game\t{cls.SEARCH_PATH}\2\n\g<0>"
        patched, count = cls.GAME_CSGO_LINE.subn(replacement, text, count=1)
        if not count:
            raise DeployError("gameinfo.gi has no `Game csgo` line; its format changed")
        return patched

    @property
    def change(self) -> str:
        if self.linked_install:
            return "template"
        return "patch" if self.patched != self.current else "ok"

    def write(self) -> None:
        """Save the patched file; CS2 reads it only at startup, so it is safe while running."""
        if self.linked_install:
            self._api.delete(self._game_dir, ["gameinfo.gi"])
        if self.patched != self.current:
            self._api.write(f"{self._game_dir}/gameinfo.gi", self.patched)
