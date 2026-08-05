#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

class ControlChannel {
private:
    SOCKET controlSocket;
    bool isConnected;

public:
    ControlChannel();
    ~ControlChannel();

    // Kết nối tới TCP Control Channel của Server
    bool connectToServer(const std::string& ip, int port);

    // Gửi lệnh FTP (ví dụ: USER, PASS, RETR, STOR)
    bool sendCommand(const std::string& command);

    // Nhận phản hồi từ Server
    std::string receiveResponse();

    // Đóng kết nối
    void disconnect();
};