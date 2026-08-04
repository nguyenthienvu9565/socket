#include "ControlChannel.h"
#include <iostream>

ControlChannel::ControlChannel() {
    controlSocket = INVALID_SOCKET;
    isConnected = false;
}   

    disconnect();
}

bool ControlChannel::connectToServer(const std::string& ip, int port) {
    controlSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (controlSocket == INVALID_SOCKET) {
        std::cout << "[TCP] Tao socket that bai!\n";
        return false;
    }

    sockaddr_in serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr)); 
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    int ptonResult = inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

    if (ptonResult == 0) {
        std::cout << "[TCP ERROR] Chuoi IP nhap vao khong dung dinh dang: " << ip << "\n";
        closesocket(controlSocket);
        controlSocket = INVALID_SOCKET;
        return false;
    } 
    else if (ptonResult < 0) {
        std::cout << "[TCP ERROR] Loi he thong khi chuyen doi IP!\n";
        closesocket(controlSocket);
        controlSocket = INVALID_SOCKET;
        return false;
    }

    if (connect(controlSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cout << "[TCP] Ket noi toi Server " << ip << ":" << port << " that bai!\n";
        closesocket(controlSocket);
        controlSocket = INVALID_SOCKET;
        return false;
    }

    isConnected = true;
    std::cout << "[TCP] Da ket noi thanh cong toi Control Channel!\n";
    return true;
}

bool ControlChannel::sendCommand(const std::string& command) {
    if (!isConnected) return false;

    std::string formattedCmd = command;
    if (formattedCmd.length() < 2 || formattedCmd.substr(formattedCmd.length() - 2) != "\r\n") {
        formattedCmd += "\r\n";
    }

    int bytesSent = send(controlSocket, formattedCmd.c_str(), formattedCmd.length(), 0);
    return bytesSent != SOCKET_ERROR;
}

std::string ControlChannel::receiveResponse() {
    if (!isConnected) return "";

    char buffer[1024] = {0};
    int bytesReceived = recv(controlSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        return std::string(buffer);
    }
    return "";
}

void ControlChannel::disconnect() {
    if (controlSocket != INVALID_SOCKET) {
        closesocket(controlSocket);
        controlSocket = INVALID_SOCKET;
    }
    isConnected = false;
}