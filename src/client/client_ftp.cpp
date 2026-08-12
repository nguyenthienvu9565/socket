#include "../../include/client/client_ftp.h"

#include <iostream>
#include <regex>

// ===================================================================
// Constructor
// Winsock KHONG khoi tao o day - rdt_init() trong main.cpp da lam.
// ===================================================================
ClientFTP::ClientFTP() {
    controlSocket = INVALID_SOCKET;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    isConnected   = false;
    serverIP      = "";
}

// ===================================================================
// Destructor
// Chi dong socket, KHONG goi WSACleanup (de rdt_cleanup() trong main)
// ===================================================================
ClientFTP::~ClientFTP() {
    disconnect();
}

// ===================================================================
// connectServer - Tao UDP socket va luu dia chi Server
//
// Phien ban 3.0: Dung SOCK_DGRAM thay vi SOCK_STREAM.
// UDP la connectionless nen khong can goi connect().
// Chi can:
//   1. Tao socket UDP
//   2. bind() vao port 0 (OS tu chon port ephemeral cho Client)
//   3. Luu serverAddr de main.cpp truyen vao rdt_send_buffer
// ===================================================================
bool ClientFTP::connectServer(const std::string& ip, int port) {
    // Tao UDP Socket
    controlSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (controlSocket == INVALID_SOCKET) {
        std::cerr << "[UDP Error] Khong tao duoc socket! Ma loi: "
                  << WSAGetLastError() << "\n";
        return false;
    }

    // Bind vao port 0 tren may Client (OS tu chon port)
    sockaddr_in localAddr;
    ZeroMemory(&localAddr, sizeof(localAddr));
    localAddr.sin_family      = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port        = 0;

    if (bind(controlSocket, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
        std::cerr << "[UDP Error] Khong bind duoc socket! Ma loi: "
                  << WSAGetLastError() << "\n";
        closesocket(controlSocket);
        controlSocket = INVALID_SOCKET;
        return false;
    }

    // Luu dia chi Server -> dung lam dest_addr khi goi rdt_send_buffer
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons((u_short)port);

    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
    if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "[UDP Error] Dia chi IP khong hop le: '" << ip << "'\n";
        closesocket(controlSocket);
        controlSocket = INVALID_SOCKET;
        return false;
    }

    serverIP    = ip;
    isConnected = true;
    std::cout << "[UDP] Da thiet lap Control Channel toi: " << ip << ":" << port << "\n";
    return true;
}

// ===================================================================
// getReplyCode - Trich xuat ma Reply Code 3 chu so
// ===================================================================
int ClientFTP::getReplyCode(const std::string& response) {
    if (response.size() < 3) return -1;
    try { return std::stoi(response.substr(0, 3)); }
    catch (...) { return -1; }
}

// ===================================================================
// parsePassiveResponse - Phan tich phan hoi PASV (Ma 227)
// "227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)"
// dataPort = p1 * 256 + p2
// ===================================================================
bool ClientFTP::parsePassiveResponse(const std::string& response,
                                      std::string& dataIP,
                                      int& dataPort) {
    std::regex pattern(R"(\((\d{1,3}),(\d{1,3}),(\d{1,3}),(\d{1,3}),(\d{1,3}),(\d{1,3})\))");
    std::smatch m;

    if (!std::regex_search(response, m, pattern)) {
        std::cerr << "[PASV Error] Khong phan tich duoc: " << response << "\n";
        return false;
    }

    dataIP = m[1].str() + "." + m[2].str() + "." + m[3].str() + "." + m[4].str();
    dataPort = std::stoi(m[5].str()) * 256 + std::stoi(m[6].str());
    return true;
}

// ===================================================================
// disconnect - Dong UDP socket
// ===================================================================
void ClientFTP::disconnect() {
    if (controlSocket != INVALID_SOCKET) {
        closesocket(controlSocket);
        controlSocket = INVALID_SOCKET;
    }
    isConnected = false;
}