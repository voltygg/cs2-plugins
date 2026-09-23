import socket
import subprocess
import time
from typing import Self

from deploy.tools.errors import DeployError


class Tunnel:
    """A background `ssh -N -L` forward that closes when its block ends."""

    TIMEOUT_SECONDS = 15

    def __init__(self, command: list[str], local_port: int) -> None:
        self.local_port = local_port
        self._process = subprocess.Popen(command)
        deadline = time.monotonic() + self.TIMEOUT_SECONDS
        while not self._accepts_connections():
            if self._process.poll() is not None:
                raise DeployError(f"SSH tunnel exited with code {self._process.returncode}")
            if time.monotonic() > deadline:
                self.close()
                raise DeployError(f"SSH tunnel did not open within {self.TIMEOUT_SECONDS}s")
            time.sleep(0.3)

    def __enter__(self) -> Self:
        return self

    def __exit__(self, *exc_info: object) -> None:
        self.close()

    def wait(self) -> None:
        self._process.wait()

    def close(self) -> None:
        self._process.terminate()

    def _accepts_connections(self) -> bool:
        try:
            socket.create_connection(("127.0.0.1", self.local_port), timeout=1).close()
        except OSError:
            return False
        return True
