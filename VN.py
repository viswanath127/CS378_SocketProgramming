import numpy as np
import socket

class VN_Graph:
    def __init__(self, N):
        self.N = N
        self.adj_matrix = np.empty()
        
class V_Node:
    def __init__(self, ON_ipaddress, UDP_Port, ip_addr = -1):
        self.ON_ip = ON_ipaddress
        self.UDP_port = UDP_Port

        if ip_addr != -1:
            self.ip_addr = ip_addr
        else:
            ... # Find it

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            self.sock.bind((self.ip_addr, self.UDP_Port))
        except Exception as e:
            print("FAILURE: ", e)
            exit(1)
    
        self.neighbours = {}

    def make_TCPconnection(self):
        self.sock.connect((self.ON_ip, 5000))

    def send_CONNECT(self):
        data = (self.ip_addr, self.UDP_port)
        data_bin = ...
        self.sock.send(data_bin)

    def receive_LSM(self, file):
        e_data = self.sock.recv(1024)
        data = e_data.decode('utf-8')
        data_list = data.strip().split()

        for i in range(len(data_list)/4):
            

        

def main():
    sock = socket.socket()