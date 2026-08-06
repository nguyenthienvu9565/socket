#include "../../include/client/client_ftp.h"
#include <iostream>
#include <sstream>

ClientFTP::ClientFTP() {
    controlSocket = INVALID_SOCKET;
    isConnected = false;

    // 1. Khởi tạo thư viện Winsock trên Windows
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[TCP Error] Khoi tao Winsock that bai!\n";
    }
}

ClientFTP::~ClientFTP() {
    disconnect();
    WSACleanup(); // Dọn dẹp tài nguyên Winsock
}

bool ClientFTP::connectServer(const std::string& ip, int port) {
    // 2. Tạo Socket TCP (SOCK_STREAM)
    controlSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (controlSocket == INVALID_SOCKET) {
        std::cerr << "[TCP Error] Tao Socket TCP that bai!\n";
        return false;
    }

    // 3. Thiết lập thông tin Server
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());

    // 4. Thực hiện kết nối TCP
    if (connect(controlSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[TCP Error] Khong the ket noi toi Server " << ip << ":" << port << "\n";
        closesocket(controlSocket);
        controlSocket = INVALID_SOCKET;
        return false;
    }

    isConnected = true;
    std::cout << "[TCP Success] Da ket noi thanh cong toi Control Channel cua Server!\n";
    return true;
}

bool ClientFTP::sendCommand(const std::string& cmd) {
    if (!isConnected || controlSocket == INVALID_SOCKET) {
        std::cerr << "[TCP Error] Chua ket noi toi Server!\n";
        return false;
    }

    // Định dạng lệnh chuẩn FTP: Luôn kết thúc bằng \r\n (CRLF)
    std::string formattedCmd = cmd;
    if (formattedCmd.length() < 2 || formattedCmd.substr(formattedCmd.length() - 2) != "\r\n") {
        formattedCmd += "\r\n";
    }

    int bytesSent = send(controlSocket, formattedCmd.c_str(), formattedCmd.length(), 0);
    if (bytesSent == SOCKET_ERROR) {
        std::cerr << "[TCP Error] Gui lenh that bai!\n";
        return false;
    }
    return true;
}

std::string ClientFTP::readResponse() {
    if (!isConnected || controlSocket == INVALID_SOCKET) return "";

    char buffer[1024] = {0};
    int bytesReceived = recv(controlSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        return std::string(buffer);
    } else if (bytesReceived == 0) {
        std::cout << "[TCP Info] Server da ngat ket noi.\n";
        disconnect();
    } else {
        std::cerr << "[TCP Error] Loi khi nhan phan hoi tu Server!\n";
    }

    return "";
}

int ClientFTP::getReplyCode(const std::string& response) {
    if (response.length() < 3) return -1;

    // Phản hồi chuẩn FTP luôn bắt đầu bằng 3 chữ số (VD: "220 Service ready", "150 Opening data connection")
    std::string codeStr = response.substr(0, 3);
    try {
        return std::stoi(codeStr);
    } catch (...) {
        return -1; // Trả về -1 nếu không phải định dạng số
    }
}

void ClientFTP::disconnect() {
    if (controlSocket != INVALID_SOCKET) {
        closesocket(controlSocket);
        controlSocket = INVALID_SOCKET;
    }
    isConnected = false;
}