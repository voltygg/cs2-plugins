import socket
import struct
import time
from typing import Self

from deploy.tools.errors import DeployError


class RconClient:
    """Minimal Source RCON client."""

    AUTH = 3
    AUTH_RESPONSE = 2
    EXEC_COMMAND = 2
    RESPONSE_VALUE = 0
    AUTH_FAILED_ID = -1

    def __init__(self, host: str, port: int, password: str, timeout: float = 10.0) -> None:
        self._socket = socket.create_connection((host, port), timeout=timeout)
        self._next_id = 10
        self._authenticate(password)

    def __enter__(self) -> Self:
        return self

    def __exit__(self, *exc_info: object) -> None:
        self.close()

    def execute(self, command: str) -> str:
        """Run one command and return its whole, possibly multi-packet, response."""
        command_id = self._next_id
        self._next_id += 2
        self._send(command_id, self.EXEC_COMMAND, command)
        # Let CS2 flush the command output before the end marker.
        time.sleep(0.25)
        self._send(command_id + 1, self.RESPONSE_VALUE, "")
        parts: list[str] = []
        while True:
            packet_id, _, body = self._receive()
            if packet_id == command_id + 1:
                return "".join(parts).strip()
            parts.append(body)

    def close(self) -> None:
        self._socket.close()

    def _authenticate(self, password: str) -> None:
        self._send(1, self.AUTH, password)
        while True:
            packet_id, packet_type, _ = self._receive()
            if packet_type != self.AUTH_RESPONSE:
                continue
            if packet_id == self.AUTH_FAILED_ID:
                raise DeployError("RCON authentication failed; check RCON_PASSWORD")
            return

    def _send(self, packet_id: int, packet_type: int, body: str) -> None:
        data = struct.pack("<ii", packet_id, packet_type) + body.encode() + b"\x00\x00"
        self._socket.sendall(struct.pack("<i", len(data)) + data)

    def _receive(self) -> tuple[int, int, str]:
        (size,) = struct.unpack("<i", self._receive_exactly(4))
        data = self._receive_exactly(size)
        packet_id, packet_type = struct.unpack("<ii", data[:8])
        return packet_id, packet_type, data[8:-2].decode(errors="replace")

    def _receive_exactly(self, size: int) -> bytes:
        data = b""
        while len(data) < size:
            chunk = self._socket.recv(size - len(data))
            if not chunk:
                raise ConnectionError("RCON connection closed by server")
            data += chunk
        return data
