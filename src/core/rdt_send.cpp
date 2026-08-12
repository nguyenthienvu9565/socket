#include "../../include/core/rdt.h"
#include "../../include/core/packet_format.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

bool rdt_init() {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
}

void rdt_cleanup() {
    WSACleanup();
}

bool rdt_send_buffer(SOCKET sock, const sockaddr_in& dest_addr, const std::vector<char>& data_buffer, int window_size, int timeout_ms) {
    std::vector<RDTPacket> packets;
    uint32_t seq = 0;
    size_t offset = 0;
    
    while (offset < data_buffer.size()) {
        RDTPacket pkt;
        memset(&pkt, 0, sizeof(pkt));
        pkt.seq_num = htonl(seq++);
        pkt.flags = htons(FLAG_DATA);
        
        size_t chunk_size = std::min((size_t)MAX_PAYLOAD_SIZE, data_buffer.size() - offset);
        memcpy(pkt.payload, data_buffer.data() + offset, chunk_size);
        
        pkt.payload_len = htons((uint16_t)chunk_size);
        pkt.checksum = calculate_checksum(&pkt);
        
        packets.push_back(pkt);
        offset += chunk_size;
    }

    uint32_t base = 0;
    uint32_t next_seq_num = 0;
    uint32_t total_packets = packets.size();
    int dest_len = sizeof(dest_addr);

    while (base < total_packets) {
        while (next_seq_num < base + window_size && next_seq_num < total_packets) {
            sendto(sock, reinterpret_cast<const char*>(&packets[next_seq_num]), sizeof(RDTPacket), 0, (struct sockaddr*)&dest_addr, dest_len);
            next_seq_num++;
        }

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);

        timeval timeout;
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;

        int activity = select(0, &read_fds, NULL, NULL, &timeout);

        if (activity > 0) {
            RDTPacket ack_pkt;
            sockaddr_in from_addr;
            int from_len = sizeof(from_addr);
            
            if (recvfrom(sock, reinterpret_cast<char*>(&ack_pkt), sizeof(RDTPacket), 0, (struct sockaddr*)&from_addr, &from_len) > 0) {
                if (calculate_checksum(&ack_pkt) == ack_pkt.checksum && ntohs(ack_pkt.flags) == FLAG_ACK) {
                    uint32_t ack_num = ntohl(ack_pkt.ack_num);
                    if (ack_num >= base) {
                        base = ack_num + 1;
                    }
                }
            }
        } else if (activity == 0) {
            next_seq_num = base; 
        }
    }

    RDTPacket fin_pkt;
    memset(&fin_pkt, 0, sizeof(fin_pkt));
    fin_pkt.seq_num = htonl(next_seq_num);
    fin_pkt.flags = htons(FLAG_FIN);
    fin_pkt.checksum = 0;
    fin_pkt.checksum = calculate_checksum(&fin_pkt);

    bool fin_acked = false;
    for (int retry = 0; retry < 5 && !fin_acked; retry++) {
        sendto(sock, reinterpret_cast<const char*>(&fin_pkt), sizeof(RDTPacket), 0, (struct sockaddr*)&dest_addr, dest_len);
        
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 300000; // Đợi ACK trong 300ms, nếu không có sẽ gửi lại FIN

        if (select(0, &read_fds, NULL, NULL, &timeout) > 0) {
            RDTPacket ack_pkt;
            sockaddr_in from_addr;
            int from_len = sizeof(from_addr);
            
            if (recvfrom(sock, reinterpret_cast<char*>(&ack_pkt), sizeof(RDTPacket), 0, (struct sockaddr*)&from_addr, &from_len) > 0) {
                uint16_t rec_chk = ack_pkt.checksum;
                ack_pkt.checksum = 0;
                if (calculate_checksum(&ack_pkt) == rec_chk && ntohs(ack_pkt.flags) == FLAG_ACK) {
                    if (ntohl(ack_pkt.ack_num) == next_seq_num) {
                        fin_acked = true;
                    }
                }
            }
        }
    }

    return true;
}

bool rdt_send_file_stream(SOCKET sock, const sockaddr_in& dest_addr, const std::string& filepath, int window_size, int timeout_ms) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Loi: Khong the mo file " << filepath << " de doc!\n";
        return false;
    }

    std::deque<RDTPacket> window;
    uint32_t base_seq = 0;
    uint32_t next_seq = 0;
    bool is_eof = false;
    int dest_len = sizeof(dest_addr);

    while (!is_eof || !window.empty()) {
        
        while (window.size() < (size_t)window_size && !is_eof) {
            RDTPacket pkt;
            memset(&pkt, 0, sizeof(pkt));
            
            file.read(pkt.payload, MAX_PAYLOAD_SIZE);
            int bytes_read = file.gcount();
            
            if (bytes_read > 0) {
                pkt.seq_num = htonl(next_seq);
                pkt.flags = htons(FLAG_DATA);
                pkt.payload_len = htons((uint16_t)bytes_read);
                pkt.checksum = calculate_checksum(&pkt);
                
                window.push_back(pkt);
                sendto(sock, reinterpret_cast<const char*>(&pkt), sizeof(RDTPacket), 0, (struct sockaddr*)&dest_addr, dest_len);
                next_seq++;
            }
            if (file.eof()) is_eof = true;
        }

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);

        timeval timeout;
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;

        int activity = select(0, &read_fds, NULL, NULL, &timeout);

        if (activity > 0) {
            RDTPacket ack_pkt;
            sockaddr_in from_addr;
            int from_len = sizeof(from_addr);
            
            if (recvfrom(sock, reinterpret_cast<char*>(&ack_pkt), sizeof(RDTPacket), 0, (struct sockaddr*)&from_addr, &from_len) > 0) {
                if (calculate_checksum(&ack_pkt) == ack_pkt.checksum && ntohs(ack_pkt.flags) == FLAG_ACK) {
                    uint32_t ack_num = ntohl(ack_pkt.ack_num);
                    if (ack_num >= base_seq) {
                        // Giải phóng các gói tin đã được xác nhận khỏi RAM
                        while (!window.empty() && ntohl(window.front().seq_num) <= ack_num) {
                            window.pop_front();
                        }
                        base_seq = ack_num + 1;
                    }
                }
            }
        } else if (activity == 0) {
            for (const auto& pkt : window) {
                sendto(sock, reinterpret_cast<const char*>(&pkt), sizeof(RDTPacket), 0, (struct sockaddr*)&dest_addr, dest_len);
            }
        }
    }
    file.close();

    RDTPacket fin_pkt;
    memset(&fin_pkt, 0, sizeof(fin_pkt));
    fin_pkt.seq_num = htonl(next_seq);
    fin_pkt.flags = htons(FLAG_FIN);
    fin_pkt.checksum = 0;
    fin_pkt.checksum = calculate_checksum(&fin_pkt);

    bool fin_acked = false;
    for (int retry = 0; retry < 5 && !fin_acked; retry++) {
        sendto(sock, reinterpret_cast<const char*>(&fin_pkt), sizeof(RDTPacket), 0, (struct sockaddr*)&dest_addr, dest_len);
        
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 300000; // Đợi ACK trong 300ms

        if (select(0, &read_fds, NULL, NULL, &timeout) > 0) {
            RDTPacket ack_pkt;
            sockaddr_in from_addr;
            int from_len = sizeof(from_addr);
            
            if (recvfrom(sock, reinterpret_cast<char*>(&ack_pkt), sizeof(RDTPacket), 0, (struct sockaddr*)&from_addr, &from_len) > 0) {
                uint16_t rec_chk = ack_pkt.checksum;
                ack_pkt.checksum = 0;
                if (calculate_checksum(&ack_pkt) == rec_chk && ntohs(ack_pkt.flags) == FLAG_ACK) {
                    if (ntohl(ack_pkt.ack_num) == next_seq) {
                        fin_acked = true;
                    }
                }
            }
        }
    }

    std::cout << "Trang thai: Truyen file hoan tat!\n";
    return true;
}