from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEPLOY_DIR = ROOT / "deploy"
PACKAGE_DIR = ROOT / "package"
RENDER_DIR = DEPLOY_DIR / ".render"
