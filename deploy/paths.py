from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEPLOY_DIR = ROOT / "deploy"
FILES_DIR = DEPLOY_DIR / "files"
PACKAGE_DIR = ROOT / "build" / "package"
RENDER_DIR = ROOT / "build" / "deploy"
