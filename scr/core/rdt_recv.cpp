#include "../../include/rdt.h"
#include "../../include/packet_format.h"
#include <iostream>
#include <fstream>

bool rdt_receive_file(SOCKET sock, const std::string& save_filepath) {
    std::ofstream file(save_filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Khong the tao file de luu!\n";
        return false;
    }

    uint32_t expected_seq = 0;
    sockaddr_in client_addr;
    int client_len = sizeof(client_addr);

    std::cout << "Dang lang nghe Data Channel...\n";

    while (true) {
        RDTPacket pkt;
        int bytes_recv = recvfrom(sock, reinterpret_cast<char*>(&pkt), sizeof(RDTPacket), 0, 
                                 (struct sockaddr*)&client_addr, &client_len);
        
        if (bytes_recv <= 0) continue;

        // Kiểm tra lỗi suy hao dữ liệu
        if (calculate_checksum(&pkt) != pkt.checksum) {
            std::cout << "Goi tin bi loi Checksum. Loai bo!\n";
            continue;
        }

        uint32_t seq_num = ntohl(pkt.seq_num);
        uint16_t flags = ntohs(pkt.flags);

        if (flags == FLAG_FIN) {
            std::cout << "Nhan duoc tin hieu ket thuc file.\n";
            break; // Thoát vòng lặp, đóng file
        }

        RDTPacket ack_pkt;
        memset(&ack_pkt, 0, sizeof(ack_pkt));
        ack_pkt.flags = htons(FLAG_ACK);

        if (seq_num == expected_seq) {
            // Nhận đúng thứ tự
            file.write(pkt.payload, ntohs(pkt.payload_len));
            std::cout << "Nhan va ghi vao file SEQ: " << seq_num << "\n";
            
            ack_pkt.ack_num = htonl(expected_seq);
            expected_seq++;
        } else {
            // Nhận sai thứ tự -> Gửi lại ACK của gói đúng gần nhất
            std::cout << "Sai thu tu (Cho " << expected_seq << ", nhan " << seq_num << "). Gui lai ACK.\n";
            if (expected_seq > 0) {
                ack_pkt.ack_num = htonl(expected_seq - 1);
            } else {
                continue; // Chưa nhận được gói 0, im lặng chờ
            }
        }

        // Gửi ACK phản hồi
        ack_pkt.checksum = calculate_checksum(&ack_pkt);
        sendto(sock, reinterpret_cast<const char*>(&ack_pkt), sizeof(RDTPacket), 0, 
              (struct sockaddr*)&client_addr, client_len);
    }

    file.close();
    return true;
}