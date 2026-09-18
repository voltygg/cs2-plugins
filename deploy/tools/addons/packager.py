"""The Linux build, staged per install component under package/."""

import shutil
import subprocess

from deploy.tools.errors import DeployError
from deploy.tools.paths import PACKAGE, ROOT


class AddonPackager:
    """Stages one built install component under package/<name> with `cmake --install`."""

    PRESET = "linux-steamrt-release"

    def __init__(self) -> None:
        self._build_dir = ROOT / "build" / self.PRESET

    def package(self, component: str) -> None:
        if not self._build_dir.is_dir():
            raise DeployError(f"no build at build/{self.PRESET}; run `voltmod build {self.PRESET}`")
        destination = PACKAGE / component
        shutil.rmtree(destination, ignore_errors=True)
        print(f"=== Packaging {component} into package/{component} ===")
        install = ["cmake", "--install", str(self._build_dir), "--component", component]
        subprocess.run([*install, "--prefix", str(destination)], check=True)
