#include<sys/socket.h>
#include<netinet/in.h>
#include<iostream>
#include<string>
#include <arpa/inet.h>
#include<utility>
#include<vector>

using namespace std;

struct nb {
    string id;
    string ip_addr;
    int UDP_Port;
    int cost;
} ;

int to_bin(string msg) {
    
}

vector<nb> decode_LSM(string msg) {

}

void decode_LSP(string msg) {

}

string nbtostr(vector<nb> neighbours) {

}

void printnb(vector<nb> neighbours) {

}

class V_Node {
    private:
        string id;
        string ON_ipaddr;
        string ip_addr;
        int ON_port;
        int UDP_port;
        sockaddr_in vn_addr;
        sockaddr_in on_addr;
        int udp_s;
        int tcp_s;
        int seq_n;
        vector<nb> neighbours;
        
    public:
        V_Node(string ip_on, int udp_port, string ip = "") {
            seq_n = 0;
            ON_ipaddr = ip_on;
            UDP_port = udp_port;

            if(/* valid(ip) */ true) {
                ip_addr = ip;
            } else {
                // find it
            }

            udp_s = socket(AF_INET, SOCK_DGRAM, 0);
            vn_addr.sin_family = AF_INET;
            vn_addr.sin_port = htons(udp_port);
            if (inet_pton(AF_INET, ip_addr.c_str(), &vn_addr.sin_addr) <= 0) {
                cerr << "Invalid address of Vritual Node";
            }

            try {
                bind(udp_s, (struct sockaddr*)&vn_addr, sizeof(vn_addr));
            } catch(...) {
                cerr << "Error";
            }
        }

        void make_tcp_connection() {
            tcp_s = socket(AF_INET, SOCK_STREAM, 0);

            on_addr.sin_family = AF_INET;
            on_addr.sin_port = htons(5000);
            if (inet_pton(AF_INET, ON_ipaddr.c_str(), &on_addr.sin_addr) <= 0) {
                cerr << "Invalid address of Oracle Node";
            }

            if (connect(tcp_s, (struct sockaddr*)&on_addr, sizeof(on_addr)) < 0) {
                cerr << "error! TCP connection not made\n";
            } else {
                cout << "TCP connection made successfully!\n";
            }
        }

        void send_connect() {
            // pair<string, int> CONNECT = {ip_addr, UDP_port};
            string CONNECT = ip_addr + to_string(UDP_port);
            int CONNECT_bin = to_bin(CONNECT); 
            send(tcp_s, to_string(CONNECT_bin).c_str(), sizeof(CONNECT_bin), 0);
        }

        void recv_LSM() {
            listen(tcp_s, 1);
            char buffer[1024] = {0};
            recv(tcp_s, buffer, sizeof(buffer), 0);

            neighbours = decode_LSM(buffer);
            printnb(neighbours);
        }

        void send_LSP() {
            for(nb neighbor: neighbours) {
                string msg = neighbor.id + to_string(seq_n) + nbtostr(neighbours);
                int bin_msg = to_bin(msg);
                send(udp_s, to_string(bin_msg).c_str(), sizeof(bin_msg), 0);
                seq_n++;
            }
        }

        void recv_LSP() {
            listen(udp_s, neighbours.size());

            for(int i=0; i<neighbours.size(); i++) {
                char buffer[1024] = {0};
                recv(udp_s, buffer, sizeof(buffer), 0);

                decode_LSP(buffer);
            }
        }
    } ;

int main(int argc, char* argv[]) {
    string ip_ON;
    int udp_port;
    string ip = "-";

    if(argc < 3) {
        cerr << "Error, not enough args\n";
    } else {
        ip_ON = argv[1];
        udp_port = stoi(argv[2]);
        if(argv[3] != "-") ip = argv[3];
    }

    V_Node VN(ip_ON, udp_port, ip);
    VN.make_tcp_connection();
    while(true) {
        VN.recv_LSM();
        VN.send_LSP();
        VN.recv_LSP();
    }
}