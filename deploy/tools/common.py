"""Shared helpers for deploy tooling."""

import shlex
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEPLOY = ROOT / "deploy"


def die(message: str) -> None:
    raise SystemExit(f"ERROR: {message}")


def repo_path(value: str | Path) -> Path:
    path = Path(value)
    return path if path.is_absolute() else ROOT / path


def command_line(args: list[str]) -> str:
    return " ".join(shlex.quote(str(arg)) for arg in args)


def run(
    args: list[str],
    *,
    cwd: Path | None = None,
    env: dict[str, str] | None = None,
    capture: bool = False,
    dry_run: bool = False,
) -> subprocess.CompletedProcess[str]:
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
