"""The Linux build, staged per plugin under package/."""

import shutil
import subprocess

from deploy.tools.errors import DeployError
from deploy.tools.paths import PACKAGE, ROOT


class PluginPackager:
    """Stages built Linux plugins under package/<plugin> with `cmake --install --component`."""

    PRESET = "linux-steamrt-release"

    def __init__(self) -> None:
        self._build_dir = ROOT / "build" / self.PRESET

    def package(self, plugin: str) -> None:
        if not self._build_dir.is_dir():
            raise DeployError(f"no build at build/{self.PRESET}; run `voltmod build {self.PRESET}`")
        destination = PACKAGE / plugin
        shutil.rmtree(destination, ignore_errors=True)
        print(f"=== Packaging {plugin} into package/{plugin} ===")
        install = ["cmake", "--install", str(self._build_dir), "--component", plugin]
        subprocess.run([*install, "--prefix", str(destination)], check=True)
