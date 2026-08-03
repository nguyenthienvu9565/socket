#pragma once

#include <cstdint>
#include <cstring>

#define MAX_PAYLOAD_SIZE 1024
#define FLAG_DATA 0
#define FLAG_ACK  1
#define FLAG_FIN  2 // Báo hiệu kết thúc truyền file

// Đảm bảo struct không bị padding bởi compiler để tính checksum chính xác
#pragma pack(push, 1)
struct RDTPacket {
    uint32_t seq_num;           // Số thứ tự gói tin
    uint32_t ack_num;           // Số thứ tự xác nhận
    uint16_t checksum;          // Mã kiểm tra lỗi
    uint16_t flags;             // Cờ đánh dấu loại gói tin (DATA, ACK, FIN)
    uint16_t payload_len;       // Độ dài dữ liệu thực tế
    char payload[MAX_PAYLOAD_SIZE]; // Dữ liệu file
};
#pragma pack(pop)

// Hàm tính Checksum cơ bản (tổng 16-bit)
inline uint16_t calculate_checksum(const RDTPacket* pkt) {
    uint32_t sum = 0;
    const uint16_t* ptr = reinterpret_cast<const uint16_t*>(pkt);
    int count = sizeof(RDTPacket) / 2;
    
    for (int i = 0; i < count; i++) {
        // Bỏ qua trường checksum khi đang tính toán chính nó
        if (i == 4) continue; 
        sum += ptr[i];
        if (sum & 0xFFFF0000) {
            sum &= 0xFFFF;
            sum++;
        }
    }
    return ~(sum & 0xFFFF);
}