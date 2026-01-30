import os
import subprocess
import sys
import time
import telefoniste

sys.path.insert(
    0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "python"))
)


SOCKET_PATH = "/tmp/telefoniste_py_test.sock"
SERVER_BIN = os.path.abspath(
    os.path.join(os.path.dirname(__file__), "py_server_test.out")
)


def wait_for_socket(path: str, timeout_s: float = 2.0) -> None:
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        if os.path.exists(path):
            return
        time.sleep(0.01)
    raise RuntimeError("socket did not appear")


def main() -> None:
    if not os.path.exists(SERVER_BIN):
        raise RuntimeError("server binary missing; build tests/py_server_test.out")

    server = subprocess.Popen([SERVER_BIN])
    try:
        wait_for_socket(SOCKET_PATH)
        sock = telefoniste.Socket(SOCKET_PATH)
        payload = b"py-client-test"
        resp = telefoniste.Client.call(sock, payload)
        assert resp == payload

        server.wait(timeout=2.0)
    finally:
        if server.poll() is None:
            server.terminate()
            try:
                server.wait(timeout=2.0)
            except subprocess.TimeoutExpired:
                server.kill()
                server.wait(timeout=2.0)


if __name__ == "__main__":
    main()
