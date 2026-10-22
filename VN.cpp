#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <string>
#include <chrono>
#include <arpa/inet.h>
#include <utility>
#include <vector>
#include <cstring>
#include <unistd.h>

using namespace std;

#pragma pack(push, 1)
struct nb {
    char id;
    uint32_t ip_addr;
    uint16_t UDP_Port;
    uint16_t cost;
};

struct CONNECT_MSG {
    uint32_t ip;
    uint16_t UDP_Port;
};

struct LSP_HDR {
    char origin_id;
    uint32_t seqnum;
    uint16_t num_nbs;
};
#pragma pack(pop)

vector<nb> decode_LSM(string msg) {
    vector<nb> neighbours;
    size_t tuple_size = sizeof(nb);
    size_t tuple_count = msg.size() / tuple_size;

    for (size_t i = 0; i < tuple_count; ++i) {
        nb tuple;
        memcpy(&tuple, msg.data() + (i * tuple_size), tuple_size);
        neighbours.push_back(tuple);
    }
    return neighbours;
}

void printnb(vector<nb> neighbours) {
    cout << "\n--- Received LINK-STATE Tuples ---" << endl;
    for (const auto& item : neighbours) {
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &item.ip_addr, ip_str, INET_ADDRSTRLEN);

        cout << "Node: " << item.id << " IP: " << ip_str 
             << " Port: " << ntohs(item.UDP_Port) 
             << " Cost: " << ntohs(item.cost) << endl;
    }
}

void decode_LSP(const char* buffer, ssize_t len) {
    if (len < (ssize_t)sizeof(LSP_HDR)) return;

    LSP_HDR hdr;
    memcpy(&hdr, buffer, sizeof(LSP_HDR));

    char origin = hdr.origin_id;
    uint32_t seq = ntohl(hdr.seqnum);
    uint16_t num_nbs = ntohs(hdr.num_nbs);

    if (len < (ssize_t)(sizeof(LSP_HDR) + num_nbs * sizeof(nb))) return;

    cout << "\n----[LSP Received] From Node: " << origin << " SeqNum: " << seq << " ----" << endl;

    const char* payload_ptr = buffer + sizeof(LSP_HDR);
    for (uint16_t i = 0; i < num_nbs; ++i) {
        nb tuple;
        memcpy(&tuple, payload_ptr + (i * sizeof(nb)), sizeof(nb));

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &tuple.ip_addr, ip_str, INET_ADDRSTRLEN);

        cout << "   Neighbor: " << tuple.id 
             << " IP: " << ip_str 
             << " Port: " << ntohs(tuple.UDP_Port) 
             << " Cost: " << ntohs(tuple.cost) << endl;
    }
}

class V_Node {
    private:
        int ON_port;
        int UDP_port;
        int udp_s;
        int tcp_s;
        int seq_n;
        char id;
        string ON_ipaddr;
        string ip_addr;
        vector<nb> neighbours;
        sockaddr_in vn_addr;
        sockaddr_in on_addr;
        
    public:
        V_Node(string ip_on, int udp_port, string ip = "") {
            seq_n = 0;
            ON_ipaddr = ip_on;
            UDP_port = udp_port;
            id = '?';

            if (ip != "" && ip != "-") {
                ip_addr = ip;
            } else {
                ip_addr = "127.0.0.1";
            }

            udp_s = socket(AF_INET, SOCK_DGRAM, 0);
            vn_addr.sin_family = AF_INET;
            vn_addr.sin_port = htons(udp_port);
            inet_pton(AF_INET, ip_addr.c_str(), &vn_addr.sin_addr);

            if (bind(udp_s, (struct sockaddr*)&vn_addr, sizeof(vn_addr)) < 0) {
                cerr << "Error binding UDP socket\n";
            }
        }

        void make_tcp_connection() {
            tcp_s = socket(AF_INET, SOCK_STREAM, 0);

            on_addr.sin_family = AF_INET;
            on_addr.sin_port = htons(5000);
            if (inet_pton(AF_INET, ON_ipaddr.c_str(), &on_addr.sin_addr) <= 0) {
                cerr << "Invalid address of Oracle Node\n";
            }

            if (connect(tcp_s, (struct sockaddr*)&on_addr, sizeof(on_addr)) < 0) {
                cerr << "ERROR! TCP connection not made. EXITING\n";
                exit(1);
            } else {
                cout << "TCP connection made successfully!\n";
            }
        }

        void send_connect() {
            CONNECT_MSG msg;
            msg.ip = vn_addr.sin_addr.s_addr;
            msg.UDP_Port = htons(UDP_port);

            send(tcp_s, &msg, sizeof(msg), 0);
            cout << "CONNECT message sent.\n";
        }

        bool recv_LSM(int f = MSG_DONTWAIT) {
            char buffer[1024] = {0};
            ssize_t bytes_recv = recv(tcp_s, buffer, sizeof(buffer), f);

            if (bytes_recv > 0) {
                string raw_msg(buffer, bytes_recv);
                neighbours = decode_LSM(raw_msg);

                for (const auto& item : neighbours) {
                    if (ntohs(item.cost) == 0) {
                        id = item.id;
                    }
                }

                printnb(neighbours);
                return true;
            } else if (f == 0 && bytes_recv <= 0) {
                cerr << "Failed to receive initial LINK-STATE message.\n";
            }

            return false;
        }

        void send_LSP() {
            if (neighbours.empty()) return;
            LSP_HDR hdr;
            hdr.origin_id = id;
            hdr.seqnum = htonl(seq_n);
            hdr.num_nbs = htons(neighbours.size());

            vector<char> packet(sizeof(LSP_HDR) + neighbours.size() * sizeof(nb));
            memcpy(packet.data(), &hdr, sizeof(LSP_HDR));
            memcpy(packet.data() + sizeof(LSP_HDR), neighbours.data(), neighbours.size() * sizeof(nb));

            for (const auto& nbr : neighbours) {
                if (ntohs(nbr.cost) == 0) continue;

                sockaddr_in dst_addr;
                memset(&dst_addr, 0, sizeof(dst_addr));
                dst_addr.sin_family = AF_INET;
                dst_addr.sin_addr.s_addr = nbr.ip_addr;
                dst_addr.sin_port = nbr.UDP_Port;

                // for (int r = 0; r < 2; r++) {
                    sendto(udp_s, packet.data(), packet.size(), 0, 
                           (struct sockaddr*)&dst_addr, sizeof(dst_addr));
                // }
            }

            cout << " LSP Sent seq " << seq_n << " from Node " << id << endl;
            seq_n++;
        }           

        void recv_LSP() {
            char buffer[1024] = {0};
            sockaddr_in src_addr;
            socklen_t addr_len = sizeof(src_addr);

            ssize_t bytes_recv = recvfrom(udp_s, buffer, sizeof(buffer), MSG_DONTWAIT,
                                          (struct sockaddr*)&src_addr, &addr_len);

            if (bytes_recv > 0) {
                decode_LSP(buffer, bytes_recv);
            }
        }
};

int main(int argc, char* argv[]) {
    string ip_ON;
    int udp_port;
    string ip = "-";

    if (argc < 3) {
        cerr << "Error, not enough args\n";
        return 1;
    } else {
        ip_ON = argv[1];
        udp_port = stoi(argv[2]);
        if (argc >= 4 && string(argv[3]) != "-") ip = argv[3];
    }

    V_Node VN(ip_ON, udp_port, ip);
    VN.make_tcp_connection();
    VN.send_connect();
    VN.recv_LSM(0);

    auto lsp_sent_l = chrono::steady_clock::now();
    VN.send_LSP();
    VN.recv_LSP();

    while (1) {
        bool lsm_recv = VN.recv_LSM();
        auto now = chrono::steady_clock::now();
        auto elapsed_sec = chrono::duration_cast<chrono::seconds>(now - lsp_sent_l).count();

        if (lsm_recv || elapsed_sec >= 15) {
            VN.send_LSP();
            lsp_sent_l = now;
        }

        VN.recv_LSP();
    }

    return 0;
}
