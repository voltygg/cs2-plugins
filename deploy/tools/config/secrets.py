import os

from dotenv import dotenv_values

from deploy.tools.errors import DeployError
from deploy.tools.paths import DEPLOY_DIR, ROOT

# CI names its own key file in the process environment; a local path copied into the file loses.
PROCESS_WINS = ("SSH_KEY_FILE",)


class ServerSecrets:
    """A server's secrets from its .env file, which CI writes; the process env fills gaps."""

    def __init__(self, server_id: str) -> None:
        self.server_id = server_id
        self.file = DEPLOY_DIR / "secrets" / server_id / ".env"
        values = dotenv_values(self.file) if self.file.is_file() else {}
        self._values = {**os.environ, **{name: value or "" for name, value in values.items()}}
        self._values |= {name: os.environ[name] for name in PROCESS_WINS if os.environ.get(name)}

    def get(self, name: str, default: str = "") -> str:
        return self._values.get(name) or default

    def require(self, name: str) -> str:
        if value := self._values.get(name):
            return value
        location = self.file.relative_to(ROOT).as_posix()
        raise DeployError(f"{name} is not set for {self.server_id}; add it to {location}")

    def as_dict(self) -> dict[str, str]:
        return dict(self._values)
