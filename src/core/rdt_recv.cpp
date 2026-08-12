#include "../../include/core/rdt.h"
#include "../../include/core/packet_format.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

std::vector<char> rdt_receive_buffer(SOCKET sock, sockaddr_in* out_addr) {
    std::vector<char> received_data;
    uint32_t expected_seq = 0;
    
    sockaddr_in client_addr;
    int client_len = sizeof(client_addr);

    while (true) {
        RDTPacket pkt;
        int bytes_recv = recvfrom(sock, reinterpret_cast<char*>(&pkt), sizeof(RDTPacket), 0, (struct sockaddr*)&client_addr, &client_len);
        
        if (bytes_recv <= 0) continue;

        uint16_t received_checksum = pkt.checksum;
        pkt.checksum = 0; // Đặt về 0 trước khi tính lại checksum gốc
        if (calculate_checksum(&pkt) != received_checksum) {
            continue; // Nếu gói tin bị lỗi thật sự thì mới bỏ qua
        }

        uint32_t seq_num = ntohl(pkt.seq_num);
        uint16_t flags = ntohs(pkt.flags);

        if (flags == FLAG_FIN) {
            RDTPacket fin_ack;
            memset(&fin_ack, 0, sizeof(fin_ack));
            fin_ack.flags = htons(FLAG_ACK);
            fin_ack.ack_num = htonl(seq_num);
            fin_ack.checksum = calculate_checksum(&fin_ack);
            sendto(sock, reinterpret_cast<const char*>(&fin_ack), sizeof(RDTPacket), 0, (struct sockaddr*)&client_addr, client_len);
            break; 
        }

        RDTPacket ack_pkt;
        memset(&ack_pkt, 0, sizeof(ack_pkt));
        ack_pkt.flags = htons(FLAG_ACK);

        if (seq_num == expected_seq) {
            uint16_t payload_len = ntohs(pkt.payload_len);
            received_data.insert(received_data.end(), pkt.payload, pkt.payload + payload_len);
            
            ack_pkt.ack_num = htonl(expected_seq);
            expected_seq++;
        } else {
            if (expected_seq > 0) ack_pkt.ack_num = htonl(expected_seq - 1);
            else continue; 
        }

        ack_pkt.checksum = calculate_checksum(&ack_pkt);
        sendto(sock, reinterpret_cast<const char*>(&ack_pkt), sizeof(RDTPacket), 0, (struct sockaddr*)&client_addr, client_len);
    }

    if (out_addr != nullptr) {
        *out_addr = client_addr;
    }

    return received_data;
}

bool rdt_receive_file_stream(SOCKET sock, const std::string& save_filepath) {
    std::ofstream file(save_filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Loi: Khong the tao file " << save_filepath << " de luu!\n";
        return false;
    }

    uint32_t expected_seq = 0;
    sockaddr_in client_addr;
    int client_len = sizeof(client_addr);

    std::cout << "Dang lang nghe luong Data Channel...\n";

    while (true) {
        RDTPacket pkt;
        int bytes_recv = recvfrom(sock, reinterpret_cast<char*>(&pkt), sizeof(RDTPacket), 0, (struct sockaddr*)&client_addr, &client_len);
        
        if (bytes_recv <= 0) continue;

        uint16_t received_checksum = pkt.checksum;
        pkt.checksum = 0; // Đặt về 0 trước khi tính lại checksum gốc
        if (calculate_checksum(&pkt) != received_checksum) {
            continue;
        }

        uint32_t seq_num = ntohl(pkt.seq_num);
        uint16_t flags = ntohs(pkt.flags);

        if (flags == FLAG_FIN) {
            std::cout << "Nhan duoc tin hieu ket thuc file.\n";
            RDTPacket fin_ack;
            memset(&fin_ack, 0, sizeof(fin_ack));
            fin_ack.flags = htons(FLAG_ACK);
            fin_ack.ack_num = htonl(seq_num);
            fin_ack.checksum = calculate_checksum(&fin_ack);
            for (int i = 0; i < 5; i++) {
                sendto(sock, reinterpret_cast<const char*>(&fin_ack), sizeof(RDTPacket), 0, (struct sockaddr*)&client_addr, client_len);
            }
            break; 
        }

        RDTPacket ack_pkt;
        memset(&ack_pkt, 0, sizeof(ack_pkt));
        ack_pkt.flags = htons(FLAG_ACK);

        if (seq_num == expected_seq) {
            file.write(pkt.payload, ntohs(pkt.payload_len));
            
            ack_pkt.ack_num = htonl(expected_seq);
            expected_seq++;
        } else {
            if (expected_seq > 0) ack_pkt.ack_num = htonl(expected_seq - 1);
            else continue; 
        }

        ack_pkt.checksum = calculate_checksum(&ack_pkt);
        sendto(sock, reinterpret_cast<const char*>(&ack_pkt), sizeof(RDTPacket), 0, (struct sockaddr*)&client_addr, client_len);
    }

    file.close();
    return true;
}