import json
import socket
import struct


class NetClient:
    def __init__(self, host: str = "127.0.0.1", port: int = 7001):
        self.host = host
        self.port = port
        self.socket = None

    def connect(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket = socket.connect(self.host, self.port)
        print(f"[Python] Connected to C++ Engine at {self.host}:{self.port}")

    def close(self):
        if self.socket:
            self.socket.close()
            print("[Python] Connection closed.")

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

    def send_transaction(self, transaction: dict) -> None:
        payload_data = json.dumps(transaction).encode("utf-8")
        payload_len = len(payload_data)
        header = struct.pack("!I", payload_len)
        self.socket.sendall(header + payload_data)

    def get_transaction(self) -> dict:
        header_bytes = self._recv_exact(4)
        payload_len = struct.unpack("!I", header_bytes)[0]
        payload_data = self._recv_exact(payload_len)
        transaction = json.loads(payload_data.decode("utf-8"))
        return transaction
