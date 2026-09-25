"""Fill the meatgg workshop addon after `poe meatgg-addon` compiles the screens into it.

Copies the compiled Stronghold content from the stronghold addon and this folder's meatgg/ files
into game/csgo_addons/meatgg. Files are added and overwritten, never deleted.
"""

import os
import shutil
from pathlib import Path

DEFAULT_CLIENT = r"C:\Program Files (x86)\Steam\steamapps\common\Counter-Strike Global Offensive"
STRONGHOLD_CONTENT = ("models", "materials", "particles", "soundevents", "sounds")


def main() -> None:
    addons = Path(os.environ.get("CS2_CLIENT_PATH", DEFAULT_CLIENT)) / "game" / "csgo_addons"
    meatgg = addons / "meatgg"
    stronghold = addons / "stronghold"
    if not stronghold.is_dir():
        raise SystemExit(f"no compiled stronghold addon at {stronghold}")

    for folder in STRONGHOLD_CONTENT:
        shutil.copytree(stronghold / folder, meatgg / folder, dirs_exist_ok=True)
    shutil.copytree(Path(__file__).parent / "meatgg", meatgg, dirs_exist_ok=True)

    print(f"Filled {meatgg}. Update the meatgg item in the Workshop Manager.")


if __name__ == "__main__":
    main()
