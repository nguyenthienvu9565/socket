#pragma once

#include <winsock2.h>
#include <string>

// Khởi tạo và dọn dẹp Winsock (gọi 1 lần ở main)
bool rdt_init();
void rdt_cleanup();

// Hàm gửi file (Dùng Go-Back-N)
bool rdt_send_file(SOCKET sock, const sockaddr_in& dest_addr, const std::string& filepath, int window_size = 4, int timeout_ms = 500);

// Hàm nhận file
bool rdt_receive_file(SOCKET sock, const std::string& save_filepath);