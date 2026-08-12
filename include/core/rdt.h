#pragma once

#include <winsock2.h>
#include <string>
#include <vector>

// Khởi tạo và dọn dẹp Winsock (chỉ gọi 1 lần ở hàm main của Client và Server)
bool rdt_init();
void rdt_cleanup();

// =====================================================================================
// API 1: XỬ LÝ DỮ LIỆU TỪ BỘ NHỚ (Dùng cho các lệnh văn bản như LIST, STAT)
// =====================================================================================

// Dùng để gửi những đoạn tin nhắn ngắn
// Hàm send và receive sẽ nhận vào và trả ra một vector 

bool rdt_send_buffer(SOCKET sock, const sockaddr_in& dest_addr, const std::vector<char>& data_buffer, int window_size = 4, int timeout_ms = 500);
std::vector<char> rdt_receive_buffer(SOCKET sock, sockaddr_in* out_addr = nullptr);


// =====================================================================================
// API 2: XỬ LÝ DỮ LIỆU TỪ Ổ CỨNG (Dùng cho các lệnh truyền file như RETR, STOR)
// =====================================================================================

// Dùng để gửi những file lớn
// Chỉ cần cung cấp đường dẫn của file tải và file lưu

bool rdt_send_file_stream(SOCKET sock, const sockaddr_in& dest_addr, const std::string& filepath, int window_size = 4, int timeout_ms = 500);
bool rdt_receive_file_stream(SOCKET sock, const std::string& save_filepath);