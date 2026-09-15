"""Repository locations the deploy tools read and write."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEPLOY = ROOT / "deploy"
PACKAGE = ROOT / "package"
RENDER = DEPLOY / ".render"
