import sys
import os
import time
import socket
import struct
import numpy as np

if len(sys.argv) < 2:
    print(f"Usage: python {sys.argv[0]} <config_file> [port]")
    sys.exit(1)

config_file = sys.argv[1]
server_port_no = int(sys.argv[2]) if len(sys.argv) >= 3 else 5000

def parse_config(filepath):
    rows = []
    with open(filepath, "r") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            tokens = line.split("#")[0].strip().split()
            if tokens:
                rows.append([int(x) for x in tokens])
    
    if not rows:
        raise ValueError("Config file contains no matrix rows.")

    n = len(rows[0]) + 1
    matrix = np.full((n, n), -1, dtype=int)
    np.fill_diagonal(matrix, 0)

    for i, row in enumerate(rows):
        for idx, cost in enumerate(row):
            j = i + 1 + idx
            if j < n:
                matrix[i][j] = cost
                matrix[j][i] = cost
    return n, matrix

N_VN, adj_matrix = parse_config(config_file)
print(f"Loaded network with {N_VN} nodes.")
print("Adjacency Matrix:\n", adj_matrix)

virtual_nodes = [None] * N_VN
connection_sockets = [None] * N_VN

# Bind to all network interfaces so external VN machines can connect
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server_socket.bind(("", server_port_no))
server_socket.listen(26)

print(f"Oracle Node listening on port {server_port_no} (bound to all network interfaces)...")

CONNECT_MSG_FORMAT = "!4s2s"
CONNECT_MSG_SIZE = struct.calcsize(CONNECT_MSG_FORMAT)

def decode_connect_message(raw_bytes: bytes) -> tuple[str, bytes]:
    if len(raw_bytes) < CONNECT_MSG_SIZE:
        raise ValueError(f"Received malformed CONNECT packet of size {len(raw_bytes)} bytes: {raw_bytes!r}")
    ip_bytes, raw_port_bytes = struct.unpack_from(CONNECT_MSG_FORMAT, raw_bytes)
    ip_str = socket.inet_ntoa(ip_bytes)
    return (ip_str, raw_port_bytes)

# Accept connections in alphabetical order (A = 0, B = 1, etc.)
for i in range(N_VN):
    conn_socket, client_addr = server_socket.accept()
    connection_sockets[i] = conn_socket
    received_bytes = conn_socket.recv(1024)
    
    try:
        virtual_nodes[i] = decode_connect_message(received_bytes)
        port_int = struct.unpack("!H", virtual_nodes[i][1])[0]
        print(f"Node {chr(ord('A') + i)} registered from {client_addr[0]}:{client_addr[1]} -> Advertised UDP ({virtual_nodes[i][0]}:{port_int})")
    except Exception as e:
        print(f"Error handling connection from {client_addr}: {e}")
        conn_socket.close()
        sys.exit(1)

# Matches VN.cpp struct nb: char id (1B), uint32_t ip (4B), uint16_t UDP_Port (2B), uint16_t cost (2B)
TUPLE_FORMAT = "!c4s2sH"

def encode_LS_message(tuples: list) -> bytes:
    packet = bytearray()
    for node_char, ip_str, port_bytes, cost in tuples:
        ip_bytes = socket.inet_aton(ip_str)
        packet.extend(struct.pack(TUPLE_FORMAT, node_char.encode('ascii'), ip_bytes, port_bytes, cost))
    return bytes(packet)

def send_LS_messages():
    """Builds and sends LINK-STATE message to all virtual nodes."""
    for i in range(N_VN):
        ls_tuples = []
        # 1. Own node identity tuple with cost 0
        node_letter = chr(ord('A') + i)
        ls_tuples.append((node_letter, virtual_nodes[i][0], virtual_nodes[i][1], 0))

        # 2. Direct neighbor tuples (cost > 0)
        for j in range(N_VN):
            if i != j and adj_matrix[i][j] > 0:
                neighbor_letter = chr(ord('A') + j)
                ls_tuples.append((neighbor_letter, virtual_nodes[j][0], virtual_nodes[j][1], int(adj_matrix[i][j])))

        encoded_msg = encode_LS_message(ls_tuples)
        try:
            connection_sockets[i].sendall(encoded_msg)
        except Exception as e:
            print(f"Error sending link-state to node {node_letter}: {e}")

print("\nAll nodes connected. Sending initial LINK-STATE messages...")
send_LS_messages()

# Monitor config file for changes
last_mtime = os.path.getmtime(config_file)
try:
    while True:
        time.sleep(1)
        curr_mtime = os.path.getmtime(config_file)
        if curr_mtime != last_mtime:
            last_mtime = curr_mtime
            try:
                _, updated_matrix = parse_config(config_file)
                adj_matrix = updated_matrix
                print("\nConfiguration file updated. Broadcasting new LINK-STATE messages...")
                print(adj_matrix)
                send_LS_messages()
            except Exception as err:
                print(f"Error reading updated config file: {err}")
except KeyboardInterrupt:
    print("\nShutting down Oracle Node.")
finally:
    for sock in connection_sockets:
        if sock:
            sock.close()
    server_socket.close()
