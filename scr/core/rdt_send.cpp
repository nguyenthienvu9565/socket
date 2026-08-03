#include "../../include/rdt.h"
#include "../../include/packet_format.h"
#include <iostream>
#include <fstream>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

bool rdt_init() {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
}

void rdt_cleanup() {
    WSACleanup();
}

bool rdt_send_file(SOCKET sock, const sockaddr_in& dest_addr, const std::string& filepath, int window_size, int timeout_ms) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Khong the mo file de doc!\n";
        return false;
    }

    // Đọc toàn bộ file vào các gói tin (Trong thực tế với file lớn >1GB, ta sẽ đọc stream, nhưng để đơn giản logic GBN, ta lưu vào vector)
    std::vector<RDTPacket> packets;
    uint32_t seq = 0;
    while (!file.eof()) {
        RDTPacket pkt;
        memset(&pkt, 0, sizeof(pkt));
        pkt.seq_num = htonl(seq++);
        pkt.flags = htons(FLAG_DATA);
        
        file.read(pkt.payload, MAX_PAYLOAD_SIZE);
        pkt.payload_len = htons((uint16_t)file.gcount());
        pkt.checksum = calculate_checksum(&pkt);
        
        packets.push_back(pkt);
    }
    file.close();

    uint32_t base = 0;
    uint32_t next_seq_num = 0;
    uint32_t total_packets = packets.size();
    
    int dest_len = sizeof(dest_addr);

    // Vòng lặp Go-Back-N
    while (base < total_packets) {
        // Gửi các gói tin trong phạm vi cửa sổ trượt
        while (next_seq_num < base + window_size && next_seq_num < total_packets) {
            sendto(sock, reinterpret_cast<const char*>(&packets[next_seq_num]), sizeof(RDTPacket), 0, 
                  (struct sockaddr*)&dest_addr, dest_len);
            std::cout << "Da gui tui tin SEQ: " << next_seq_num << "\n";
            next_seq_num++;
        }

        // Cài đặt Timeout bằng select()
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);

        timeval timeout;
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;

        // Bỏ qua các điều kiện gửi và điều kiện lỗi (NULL, NULL)
        // Chỉ cần tập trung canh chừng cổng nhận dữ liệu (&read_fds), khoảng thời gian (&timeout)
        int activity = select(0, &read_fds, NULL, NULL, &timeout);

        if (activity > 0) { // Có dữ liệu đến (ACK)
            RDTPacket ack_pkt;
            sockaddr_in from_addr;
            int from_len = sizeof(from_addr);
            
            int bytes_recv = recvfrom(sock, reinterpret_cast<char*>(&ack_pkt), sizeof(RDTPacket), 0, (struct sockaddr*)&from_addr, &from_len);
            
            // Bắt buộc phải kiểm tra bytes_recv > 0
            if (bytes_recv > 0) {
                if (calculate_checksum(&ack_pkt) == ack_pkt.checksum && ntohs(ack_pkt.flags) == FLAG_ACK) {
                    uint32_t ack_num = ntohl(ack_pkt.ack_num);
                    std::cout << "Nhan duoc ACK: " << ack_num << "\n";
                    // Cumulative ACK: Trượt cửa sổ lên
                    if (ack_num >= base) {
                        base = ack_num + 1;
                    }
                }
            } else {
                // In ra lỗi để debug thay vì bị kẹt timeout im lặng
                std::cerr << "Loi nhan ACK (Thuong do Server chua bat). Ma loi: " << WSAGetLastError() << "\n";
            }
        }
    }

    // Gửi gói tin FIN báo hiệu kết thúc
    RDTPacket fin_pkt;
    memset(&fin_pkt, 0, sizeof(fin_pkt));
    fin_pkt.seq_num = htonl(next_seq_num);
    fin_pkt.flags = htons(FLAG_FIN);
    fin_pkt.checksum = calculate_checksum(&fin_pkt);
    sendto(sock, reinterpret_cast<const char*>(&fin_pkt), sizeof(RDTPacket), 0, (struct sockaddr*)&dest_addr, dest_len);

    std::cout << "Trang thai: Truyen file hoan tat!\n";
    return true;
}