import json
import socket
import struct
import threading

import pytest

from eth_analyzer.net_client import NetClient


def recv_n(self_socket: socket, n: int):
    data = bytearray()
    while len(data) < n:
        packet = self_socket.recv(n - len(data))
        if not packet:
            raise ConnectionError(
                "Connection closed before receiving all expected bytes."
            )
        data.extend(packet)
    return bytes(data)


@pytest.fixture
def mock_cpp_engine():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(("127.0.0.1", 0))
    server_socket.listen(1)
    host, port = server_socket.getsockname()

    def run_echo_server():
        server_socket.settimeout(2.0)
        try:
            conn, _ = server_socket.accept()
            with conn:
                header = recv_n(conn, 4)
                if not header:
                    return
                header = struct.unpack("!I", header)[0]
                payload = recv_n(conn, header)
                text = json.loads(payload.decode("utf-8"))
                text["status"] = "processed_by_mock"
                response_payload = json.dumps(text).encode("utf-8")
                response_header = struct.pack("!I", len(response_payload))
                conn.sendall(response_header + response_payload)
        except socket.timeout:
            pass
        finally:
            server_socket.close()

    server_thread = threading.Thread(target=run_echo_server)
    server_thread.start()
    yield host, port
    server_thread.join()


def test_net_client_context_manager_integration(mock_cpp_engine):
    host, port = mock_cpp_engine
    with NetClient(host=host, port=port) as client:
        test_payload = {"hash": "0xabc", "value": 10}
        client.send_json(test_payload)

        response = client.get_json()
        assert response["hash"] == "0xabc"
        assert response["status"] == "processed_by_mock"
    assert client.socket is None
