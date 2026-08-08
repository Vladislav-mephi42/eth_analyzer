import json
import logging
import socket
import struct

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
)


def sum(a: int, b: int) -> int:
    return a + b


class NetClient:
    def __init__(self, host: str = "127.0.0.1", port: int = 7001):
        self.host = host
        self.port = port
        self.socket = None

    def connect(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((self.host, self.port))
        logging.info("[Python] Connected to C++ Engine at %s:%s", self.host, self.port)

    def close(self):
        if self.socket:
            try:
                self.socket.close()
                logging.info("[Python] Connection closed.")
            finally:
                self.socket = None

    def __del__(self):
        self.close()

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def _recv_exact(self, n: int) -> bytes:
        data = bytearray()
        while len(data) < n:
            packet = self.socket.recv(n - len(data))
            if not packet:
                raise ConnectionError(
                    "Connection closed before receiving all expected bytes."
                )
            data.extend(packet)
        return bytes(data)

    def send_json(self, transaction: dict) -> None:
        payload_data = json.dumps(transaction).encode("utf-8")
        payload_len = len(payload_data)
        header = struct.pack("!I", payload_len)
        self.socket.sendall(header + payload_data)

    def get_json(self) -> dict:
        header_bytes = self._recv_exact(4)
        payload_len = struct.unpack("!I", header_bytes)[0]
        payload_data = self._recv_exact(payload_len)
        transaction = json.loads(payload_data.decode("utf-8"))
        return transaction
