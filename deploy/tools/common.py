"""Shared helpers for deploy tooling."""

from __future__ import annotations

import shlex
import subprocess
from pathlib import Path
from typing import NoReturn

ROOT = Path(__file__).resolve().parents[2]
DEPLOY = ROOT / "deploy"


# Duplicates buildtools.die on purpose: CI runs deploy tooling without submodules.
def die(message: str) -> NoReturn:
    """Exit the current command with a deploy-tooling error message."""
    raise SystemExit(f"ERROR: {message}")


def repo_path(value: str | Path) -> Path:
    """Resolve a path relative to the repository root when needed."""
    path = Path(value)
    return path if path.is_absolute() else ROOT / path


def command_line(args: list[str]) -> str:
    """Return a shell-quoted command string for display or nested SSH tools."""
    return " ".join(shlex.quote(str(arg)) for arg in args)


def run(
    args: list[str],
    *,
    cwd: Path | None = None,
    env: dict[str, str] | None = None,
    capture: bool = False,
    dry_run: bool = False,
) -> subprocess.CompletedProcess[str]:
    """Run a subprocess, optionally printing it instead for dry-run previews."""
    if dry_run:
        print(f"DRY: {command_line(args)}")
        return subprocess.CompletedProcess(args, 0, "", "")
    return subprocess.run(
        args,
        cwd=cwd,
        env=env,
        check=True,
        text=True,
        capture_output=capture,
    )


def load_server_env(server_id: str, *, required: bool) -> None:
    """Load deploy/secrets/servers/<id>/.env into process environment."""
    env_file = DEPLOY / "secrets" / "servers" / server_id / ".env"
    if not env_file.is_file():
        if required:
            die(f"no env for {server_id} (expected {env_file})")
        return

    try:
        from dotenv import load_dotenv
    except ImportError as exc:
        raise SystemExit(
            "ERROR: python-dotenv is required to load server env files; run through `uv run`."
        ) from exc

    load_dotenv(env_file, override=False)
