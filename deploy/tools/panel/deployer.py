"""Deploys to panel servers."""

import io
import sys
import tarfile
from collections.abc import Generator
from contextlib import contextmanager
from functools import cached_property

from deploy.tools.config.servers import Instance, PanelServer
from deploy.tools.deployer import Deployer
from deploy.tools.errors import DeployError
from deploy.tools.panel.api import PanelApi
from deploy.tools.panel.gameinfo import GameInfo
from deploy.tools.panel.metamod import Metamod
from deploy.tools.paths import RENDER


class PanelDeployer(Deployer[PanelServer]):
    """Uploads plugins through the panel API and restarts the server around the change."""

    @cached_property
    def api(self) -> PanelApi:
        return PanelApi(self.server, self.env.require("PANEL_API_KEY"))

    def deploy(self) -> None:
        instance = self.server.instance()
        game_dir = self.server.game_dir
        archive = self._plugin_archive(instance)
        gameinfo = GameInfo.read(self.api, game_dir)
        metamod = Metamod.read(self.api, game_dir)
        plugins = " ".join(self.server.plugins_for(instance)) or "<none>"
        print(f"=== Deploying {self.server.id} ({self.server.panel_url}, {self.api.state()}) ===")
        print(f"    plugins:  {plugins} ({len(archive) // 1024} KiB)")
        print(f"    metamod:  {metamod.wanted}{'' if metamod.outdated else ' (installed)'}")
        print(f"    gameinfo: {gameinfo.change}")
        if self.dry_run:
            print("=== Dry run complete; the server was not changed ===")
            return

        metamod.download()
        gameinfo.write()
        self._stop()
        try:
            metamod.install()
            self.api.extract(game_dir, "cs2-plugins.tar.gz", archive)
            self._remove_unused_plugins(instance)
        except Exception:
            # A server running a partial install beats one left offline.
            print("ERROR: deploy failed; starting the server again", file=sys.stderr)
            self._start_after_failure()
            raise
        self._start()
        print(f"=== Deploy to {self.server.id} complete ===")

    def update(self) -> None:
        if self.dry_run:
            print(f"DRY: restart {self.server.id} through {self.server.panel_url}")
            return
        self._stop()
        self._start()
        print(f"=== Update of {self.server.id} complete ===")

    @contextmanager
    def rcon_address(self, instance: Instance) -> Generator[tuple[str, int]]:
        # Panel servers expose RCON on the public game port.
        yield self.server.host, instance.port

    def _plugin_archive(self, instance: Instance) -> bytes:
        addons = self.addons.build(instance, RENDER / self.server.id)
        buffer = io.BytesIO()
        with tarfile.open(fileobj=buffer, mode="w:gz") as tar:
            tar.add(addons, arcname="addons")
        return buffer.getvalue()

    def _remove_unused_plugins(self, instance: Instance) -> None:
        unused = self.unused_plugin_paths(instance)
        if removed := self.api.delete(f"{self.server.game_dir}/addons", unused):
            print(f"    removed unassigned plugins: {' '.join(removed)}")

    def _stop(self) -> None:
        # Overwriting a loaded .so can crash a running server.
        if self.api.state() == "offline":
            return
        print("    stopping server")
        if not self.api.power("stop", "offline", 60) and not self.api.power("kill", "offline", 30):
            raise DeployError("server did not stop")

    def _start(self) -> None:
        print("    starting server")
        if not self.api.power("start", "running", 600):
            raise DeployError("server did not start within 10 minutes; check the panel console")

    def _start_after_failure(self) -> None:
        """Start the server without hiding the error that stopped the deploy."""
        try:
            self._start()
        except DeployError as error:
            print(f"ERROR: {error}", file=sys.stderr)
