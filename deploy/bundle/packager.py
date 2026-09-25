import shutil
import subprocess

from deploy import console
from deploy.errors import DeployError
from deploy.paths import PACKAGE_DIR, ROOT


class AddonPackager:
    """Stages one built install component under build/package/<name> with `cmake --install`."""

    PRESET = "linux-steamrt-release"

    def __init__(self) -> None:
        self._build_dir = ROOT / "build" / self.PRESET

    def package(self, component: str) -> None:
        if not self._build_dir.is_dir():
            raise DeployError(
                f"no build at build/{self.PRESET}; run `voltmod build -p {self.PRESET}`"
            )
        destination = PACKAGE_DIR / component
        shutil.rmtree(destination, ignore_errors=True)
        console.section(f"Packaging {component} into build/package/{component}")
        install = ["cmake", "--install", str(self._build_dir), "--component", component]
        subprocess.run([*install, "--prefix", str(destination)], check=True)
