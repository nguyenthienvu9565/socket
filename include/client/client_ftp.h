#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

// ===================================================================
// ClientFTP - Module quan ly Kenh Dieu khien (Control Channel)
//
// Phien ban 3.0 - Thong nhat giao tiep qua RDT:
//   - controlSocket la SOCK_DGRAM (UDP), KHONG con SOCK_STREAM (TCP)
//   - Moi giao tiep deu qua rdt_send_buffer / rdt_receive_buffer
//   - main.cpp lay getSocket() + getServerAddr() roi goi ham RDT
//   - Winsock khoi tao 1 lan duy nhat boi rdt_init() trong main.cpp
//
// Cac ham chinh giu lai:
//   - connectServer : tao UDP socket + luu dia chi Server
//   - getSocket     : tra ve SOCKET cho ham RDT
//   - getServerAddr : tra ve sockaddr_in cua Server (dung lam dest_addr cho rdt_send_buffer)
//   - getReplyCode  : tach ma 3 chu so tu chuoi phan hoi
//   - parsePassiveResponse : phan tich ma 227 de lay IP:Port Data Channel
// ===================================================================
class ClientFTP {
private:
    SOCKET      controlSocket;   // UDP socket giao tiep voi Server
    sockaddr_in serverAddr;      // Dia chi Server (dung lam dest_addr trong rdt_send_buffer)
    bool        isConnected;
    std::string serverIP;

public:
    ClientFTP();
    ~ClientFTP();

    // Tao UDP socket va luu dia chi Server
    // ip   : dia chi IPv4 (VD: "192.168.1.10")
    // port : port Control Channel cua Server (VD: 2121)
    // Tra ve true neu thanh cong
    bool connectServer(const std::string& ip, int port);

    // Tra ve UDP socket de main.cpp truyen vao cac ham rdt_send_buffer / rdt_receive_buffer
    SOCKET getSocket() const { return controlSocket; }

    // Tra ve dia chi Server (dest_addr) de truyen vao rdt_send_buffer khi gui lenh
    const sockaddr_in& getServerAddr() const { return serverAddr; }

    // Lay IP Server (dung de dien dest_addr khi mo Data Channel sau PASV)
    const std::string& getServerIP() const { return serverIP; }

    // Tach lay ma Reply Code 3 chu so tu chuoi phan hoi
    // VD: "220 Service Ready" -> 220 | "150 Opening data connection" -> 150
    // Tra ve -1 neu khong hop le
    int getReplyCode(const std::string& response);

    // Phan tich phan hoi PASV (ma 227) de lay IP va Port cua Data Channel
    // VD: "227 Entering Passive Mode (192,168,1,1,19,136)"
    //     -> dataIP = "192.168.1.1", dataPort = 19*256 + 136 = 5000
    bool parsePassiveResponse(const std::string& response,
                               std::string& dataIP,
                               int& dataPort);

    // Kiem tra trang thai ket noi
    bool connected() const { return isConnected; }

    // Dong UDP socket va dat lai trang thai
    void disconnect();
};