import sys
import numpy as np
import socket
config_file = sys.argv[1]

N_VN =0

with open(config_file, "r") as f:
    r1_found = False
    i=0
    for l in f:
        j=i+1
        ll = l.strip().split()
        if ll[0] == '#':
            continue
        if not r1_found:
            r1_found = True
            N_VN = len(ll)+1
        else : break

adj_matrix = np.full((N_VN, N_VN),0)

with open(config_file, "r") as f:
    r1_found = False
    i=0
    for l in f:
        j=i+1
        ll = l.strip().split()
        if ll[0] == '#':
            continue
        for x in ll:
            adj_matrix[i][j] = x
            adj_matrix[j][i] = x
            j = j+1
        i = i+1
print(adj_matrix)

virtual_nodes = [None] * N_VN
connection_sockets = [None] * N_VN
# print(virtual_nodes)

server_addr = "0"
server_port_no = 5000

server_socket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)

server_socket.bind((server_addr,server_port_no))

server_socket.listen(26)

def decode_from_binary_to_clientaddr(bytes):
    client_addr
    port_no
    return (client_addr, port_no)

# def send_LS_message():

def encode_LS_message():

for i in range(N_VN):
    conn_socket, client_addr = server_socket.accept()
    connection_sockets[i] = conn_socket

    received_bytes = conn_socket.recv(1024)

    virtual_nodes[i] = decode_from_binary_to_clientaddr(received_bytes)

for i in range(N_VN):
    LS_message = []
    for j in range(N_VN):
        if adj_matrix[i][j]>=0 :
            LS_message.append((i,virtual_nodes[i][0], virtual_nodes[i][1],adj_matrix[i][j]))
    encoded_LS_message = encode_LS_message(LS_message)

    connection_sockets[i].sendall(encode_LS_message)

















