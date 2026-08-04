#include "DataChannel.h"
#include <iostream>
#include <fstream>
#include <cstring>

DataChannel::DataChannel() {
    dataSocket = INVALID_SOCKET;
    dataPort = 0;
}

DataChannel::~DataChannel() {
    closeUDP();
}

bool DataChannel::initUDP(int port) {
    // 1. Tạo Socket UDP (SOCK_DGRAM)
    dataSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (dataSocket == INVALID_SOCKET) {
        std::cout << "[UDP] Tao Socket UDP that bai!\n";
        return false;
    }

    // 2. Bind Socket với Port local của Client
    sockaddr_in clientAddr;
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(port);
    clientAddr.sin_addr.s_addr = INADDR_ANY; // Nhận từ bất kỳ card mạng nào

    if (bind(dataSocket, (sockaddr*)&clientAddr, sizeof(clientAddr)) == SOCKET_ERROR) {
        std::cout << "[UDP] Bind Port UDP " << port << " that bai!\n";
        closesocket(dataSocket);
        dataSocket = INVALID_SOCKET;
        return false;
    }

    this->dataPort = port;
    std::cout << "[UDP] Da khoi tao Kenh du lieu UDP tai Port: " << port << std::endl;
    return true;
}

bool DataChannel::receiveFile(const std::string& saveFilePath, const std::string& serverIP, int serverPort) {
    if (dataSocket == INVALID_SOCKET) return false;

    std::ofstream outFile(saveFilePath, std::ios::binary);
    if (!outFile.is_open()) {
        std::cout << "[UDP] Khong the tao file de luu: " << saveFilePath << std::endl;
        return false;
    }

    sockaddr_in serverAddr;
    int addrLen = sizeof(serverAddr);
    char buffer[BUFFER_SIZE];

    std::cout << "[UDP] Dang nhan file tu Server...\n";

    // Vòng lặp nhận các gói UDP
    while (true) {
        // Nhận gói tin từ Server
        int bytesReceived = recvfrom(dataSocket, buffer, BUFFER_SIZE, 0, (sockaddr*)&serverAddr, &addrLen);
        
        if (bytesReceived <= 0) {
            std::cout << "[UDP] Loi hoac ket thuc nhan du lieu.\n";
            break;
        }

        // Kiểm tra xem IP và Port có khớp với Server không
        if (serverAddr.sin_addr.s_addr != inet_addr(serverIP.c_str()) || ntohs(serverAddr.sin_port) != serverPort) {
            continue; // Bỏ qua gói tin rác từ nguồn khác
        }

        // Ước định tín hiệu kết thúc: ví dụ Server gửi gói tin 0 bytes hoặc chuỗi "EOF"
        if (bytesReceived == 3 && strncmp(buffer, "EOF", 3) == 0) {
            std::cout << "[UDP] Nhan tin hieu ket thuc file (EOF) tu Server.\n";
            break;
        }

        // Ghi dữ liệu nhận được vào File
        outFile.write(buffer, bytesReceived);
    }

    outFile.close();
    std::cout << "[UDP] Tai file thanh cong va luu vao: " << saveFilePath << std::endl;
    return true;
}

bool DataChannel::sendFile(const std::string& sourceFilePath, const std::string& serverIP, int serverPort) {
    if (dataSocket == INVALID_SOCKET) return false;

    std::ifstream inFile(sourceFilePath, std::ios::binary);
    if (!inFile.is_open()) {
        std::cout << "[UDP] Khong the mo file de gui: " << sourceFilePath << std::endl;
        return false;
    }

    // Thiết lập địa chỉ UDP Server nhận dữ liệu
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());

    char buffer[BUFFER_SIZE];
    std::cout << "[UDP] Dang gui file sang Server...\n";

    // Đọc từng khối file và gửi qua UDP
    while (!inFile.eof()) {
        inFile.read(buffer, BUFFER_SIZE);
        std::streamsize bytesRead = inFile.gcount();

        if (bytesRead > 0) {
            if (sendto(dataSocket, buffer, bytesRead, 0, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                std::cout << "[UDP] Loi khi gui du lieu. Ma loi: " << WSAGetLastError() << "\n";
                break;
            }
        }
    }

    inFile.close();

    // Gửi gói tin báo hiệu kết thúc file ("EOF")
    sendto(dataSocket, "EOF", 3, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));

    std::cout << "[UDP] Gui file hoan tat!\n";
    return true;
}

void DataChannel::closeUDP() {
    if (dataSocket != INVALID_SOCKET) {
        closesocket(dataSocket);
        dataSocket = INVALID_SOCKET;
    }
}