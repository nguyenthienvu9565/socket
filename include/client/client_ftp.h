#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

class ClientFTP {
private:
    SOCKET controlSocket;
    bool isConnected;

public:
    ClientFTP();
    ~ClientFTP();

    bool connectServer(const std::string& ip, int port);
    bool sendCommand(const std::string& cmd);
    std::string readResponse();
    
    // Tách lấy mã 3 chữ số từ phản hồi của Server (ví dụ: "220 Service Ready" -> 220)
    int getReplyCode(const std::string& response);
    
    void disconnect();
};
