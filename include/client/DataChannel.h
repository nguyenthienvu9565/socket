#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

// Kích thước của mỗi khối dữ liệu truyền qua UDP (ví dụ: 1024 bytes)
const int BUFFER_SIZE = 1024;

class DataChannel {
private:
    SOCKET dataSocket;
    int dataPort;

public:
    DataChannel();
    ~DataChannel();

    // Khởi tạo UDP Socket và Bind vào một port trên Client
    bool initUDP(int port);

    // Nhận file từ Server qua UDP (Dùng cho lệnh RETR - Download)
    bool receiveFile(const std::string& saveFilePath, const std::string& serverIP, int serverPort);

    // Gửi file sang Server qua UDP (Dùng cho lệnh STOR - Upload)
    bool sendFile(const std::string& sourceFilePath, const std::string& serverIP, int serverPort);

    // Đóng Socket UDP
    void closeUDP();
};

