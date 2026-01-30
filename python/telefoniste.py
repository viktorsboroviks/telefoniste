"""Minimal client for telefoniste.

Protocol:
- 4-byte big-endian length header
- payload bytes
- one request per connection (connect -> send -> recv -> close)
"""

import os
import socket
import struct

DEFAULT_SOCKET_PATH = "/tmp/telefoniste.sock"
HEADER_STRUCT = struct.Struct("!I")  # network byte order uint32


class TelefonisteError(RuntimeError):
    pass


class Socket:
    def __init__(self, path: str = DEFAULT_SOCKET_PATH) -> None:
        self.path = path
        self.sock = None

    def __enter__(self) -> "Socket":
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        self.close()

    def open_client(self) -> None:
        if self.sock is not None:
            raise TelefonisteError("socket already open")
        if not os.path.exists(self.path):
            raise TelefonisteError(f"socket path does not exist: {self.path}")
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.connect(self.path)

    def close(self) -> None:
        if self.sock is None:
            return
        try:
            self.sock.close()
        finally:
            self.sock = None

    @staticmethod
    def write(sock: socket.socket, payload: bytes) -> None:
        header = HEADER_STRUCT.pack(len(payload))
        sock.sendall(header)
        if payload:
            sock.sendall(payload)

    @staticmethod
    def readn(sock: socket.socket, n: int) -> bytes:
        if n == 0:
            return b""
        chunks = []
        remaining = n
        while remaining:
            chunk = sock.recv(remaining)
            if not chunk:
                break
            chunks.append(chunk)
            remaining -= len(chunk)
        return b"".join(chunks)

    @staticmethod
    def read(sock: socket.socket) -> bytes:
        header = Socket.readn(sock, HEADER_STRUCT.size)
        if len(header) != HEADER_STRUCT.size:
            raise TelefonisteError("unexpected EOF reading header")
        (msg_len,) = HEADER_STRUCT.unpack(header)
        if msg_len == 0:
            return b""
        body = Socket.readn(sock, msg_len)
        if len(body) != msg_len:
            raise TelefonisteError("unexpected EOF reading body")
        return body

    def send_msg(self, payload: bytes) -> None:
        if self.sock is None:
            raise TelefonisteError("socket not open")
        self.write(self.sock, _ensure_bytes(payload))

    def receive_msg(self) -> bytes:
        if self.sock is None:
            raise TelefonisteError("socket not open")
        return self.read(self.sock)


class Client:
    @staticmethod
    def call(sock: Socket, payload: bytes) -> bytes:
        sock.open_client()
        try:
            sock.send_msg(payload)
            return sock.receive_msg()
        finally:
            sock.close()


def _ensure_bytes(payload: bytes | bytearray | memoryview) -> bytes:
    if not isinstance(payload, (bytes, bytearray, memoryview)):
        raise TypeError("payload must be bytes-like")
    return bytes(payload)
