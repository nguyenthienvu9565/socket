#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

// ===================================================================
// ClientFTP - Module quan ly Kenh Dieu khien TCP (Control Channel)
//
// Chuc nang chinh:
//   - Ket noi toi Server FTP qua TCP (Socket SOCK_STREAM)
//   - Gui lenh FTP dinh dang chuan (ket thuc CRLF \r\n)
//   - Doc phan hoi tu Server va trich xuat ma Reply Code 3 chu so
//   - Phan tich phan hoi PASV de lay IP:Port cua Data Channel
//   - Xay dung lenh PORT cho che do Active
//
// Luu y ky thuat:
//   - Winsock (WSAStartup/WSACleanup) duoc quan ly trong constructor/destructor
//   - Dung inet_pton thay inet_addr: an toan hon, ho tro ca IPv4 va IPv6
//   - ZeroMemory dam bao khong co rac bo nho trong cau truc sockaddr_in
// ===================================================================
class ClientFTP {
private:
    SOCKET      controlSocket;  // Socket TCP ket noi kieu Dieu khien
    bool        isConnected;    // Co ket noi dang hoat dong khong
    std::string serverIP;       // IP Server: dung khi xay dung lenh PORT

public:
    ClientFTP();
    ~ClientFTP();

    // Ket noi toi Server FTP qua TCP Control Channel
    // ip   : dia chi IPv4 dang chuoi (VD: "192.168.1.10")
    // port : cong dich vu (thuong la 21 hoac 2121)
    // Tra ve true neu ket noi thanh cong
    bool connectServer(const std::string& ip, int port);

    // Gui lenh FTP len Server qua TCP (tu dong them \r\n neu thieu)
    // cmd : lenh da dinh dang (VD: "RETR myfile.pdf")
    // Tra ve true neu gui thanh cong
    bool sendCommand(const std::string& cmd);

    // Doc mot phan hoi tu Server qua TCP
    // Tra ve chuoi phan hoi, hoac chuoi rong neu loi/ngat ket noi
    std::string readResponse();

    // Tach lay ma 3 chu so tu chuoi phan hoi
    // VD: "220 Service Ready" -> 220
    //     "150 Opening data connection" -> 150
    // Tra ve -1 neu khong phai dinh dang FTP hop le
    int getReplyCode(const std::string& response);

    // Phan tich phan hoi PASV cua Server de lay Data Channel IP va Port
    // VD: "227 Entering Passive Mode (192,168,1,1,19,136)"
    //     -> dataIP = "192.168.1.1", dataPort = 19*256 + 136 = 5000
    // Cong thuc port: dataPort = p1 * 256 + p2
    // Tra ve true neu phan tich thanh cong
    bool parsePassiveResponse(const std::string& response,
                               std::string& dataIP,
                               int& dataPort);

    // Xay dung chuoi lenh PORT tu IP cuc bo va port duoc chi dinh
    // Dinh dang FTP: "PORT h1,h2,h3,h4,p1,p2"
    //   o do h1-h4 la 4 octet cua IP, p1 = port/256, p2 = port%256
    // VD: IP=192.168.1.10, port=5000 -> "PORT 192,168,1,10,19,136"
    // Tra ve chuoi lenh, hoac rong neu gap loi
    std::string buildPortCommand(int localPort);

    // Lay dia chi IP Server dang ket noi (de dung trong Data Channel)
    const std::string& getServerIP() const { return serverIP; }

    // Kiem tra xem Client co dang ket noi khong
    bool connected() const { return isConnected; }

    // Ngat ket noi va dong socket TCP
    void disconnect();
};
