#include "../../include/client/client_ftp.h"

#include <iostream>
#include <sstream>
#include <regex>    // std::regex: dung de phan tich chuoi PASV

// ===================================================================
// Constructor - Khoi tao doi tuong ClientFTP
// Dieu quan trong: Winsock PHAI duoc khoi tao truoc bat ky ham socket
// nao duoc goi. Dat WSAStartup o day dam bao thu tu khoi tao chinh xac.
// ===================================================================

ClientFTP::ClientFTP() {
    controlSocket = INVALID_SOCKET;
    isConnected   = false;
    serverIP      = "";

    // Khoi tao thu vien Winsock 2.2 tren Windows
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "[TCP Error] WSAStartup that bai! Ma loi: " << result << "\n";
    }
}

// ===================================================================
// Destructor - Don dep tai nguyen khi doi tuong bi huy
// Goi disconnect() truoc WSACleanup de dam bao thu tu don dep hop le
// ===================================================================
ClientFTP::~ClientFTP() {
    disconnect();
    WSACleanup();
}

// ===================================================================
// connectServer - Thiet lap ket noi TCP toi Server
//
// Cac buoc thuc hien:
//   1. Tao socket TCP (AF_INET, SOCK_STREAM, IPPROTO_TCP)
//   2. Khoi tao sockaddr_in bang ZeroMemory de xoa rac bo nho
//   3. Dung inet_pton (thay inet_addr cu) de chuyen IP sang nhi phan
//      inet_pton tra ve: 1=thanh cong, 0=IP sai dinh dang, -1=loi he thong
//   4. Goi connect() de thiet lap ket noi TCP 3-way handshake
// ===================================================================
bool ClientFTP::connectServer(const std::string& ip, int port) {
    // Tao TCP Socket (SOCK_STREAM = kieu luong byte, dam bao thu tu)
    controlSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (controlSocket == INVALID_SOCKET) {
        std::cerr << "[TCP Error] Khong tao duoc socket TCP!"
                  << " Ma loi Winsock: " << WSAGetLastError() << "\n";
        return false;
    }

    // Khoi tao cau truc dia chi Server ve tat ca 0 truoc khi gan gia tri
    // Day la thao tac bat buoc de tranh rac trong cac truong khong dung den
    sockaddr_in serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons((u_short)port);  // htons: chuyen sang Network Byte Order

    // Chuyen chuoi IP sang dang nhi phan voi inet_pton (Modern Socket API)
    // Uu diem so voi inet_addr: ho tro IPv6, kiem tra loi ro rang hon
    int convResult = inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);
    if (convResult == 0) {
        // Chuoi IP nhap vao khong dung dinh dang IPv4
        std::cerr << "[TCP Error] Dia chi IP khong hop le: '" << ip << "'\n";
        std::cerr << "            Hay nhap theo dinh dang IPv4, VD: 192.168.1.10\n";
        closesocket(controlSocket);
        controlSocket = INVALID_SOCKET;
        return false;
    } else if (convResult < 0) {
        // Loi cap do he thong (hiem gap)
        std::cerr << "[TCP Error] Loi he thong khi phan tich IP!"
                  << " Ma loi: " << WSAGetLastError() << "\n";
        closesocket(controlSocket);
        controlSocket = INVALID_SOCKET;
        return false;
    }

    // Thiet lap ket noi TCP (TCP 3-way handshake)
    if (connect(controlSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[TCP Error] Khong the ket noi toi " << ip << ":" << port << "\n";
        std::cerr << "            Ma loi Winsock: " << WSAGetLastError() << "\n";
        std::cerr << "            Kiem tra lai: Server co dang chay? Firewall? IP/Port dung?\n";
        closesocket(controlSocket);
        controlSocket = INVALID_SOCKET;
        return false;
    }

    // Luu IP Server de dung sau trong buildPortCommand()
    serverIP    = ip;
    isConnected = true;
    std::cout << "[TCP] Da ket noi toi Control Channel: " << ip << ":" << port << "\n";
    return true;
}

// ===================================================================
// sendCommand - Gui lenh FTP len Server qua TCP
//
// Giao thuc FTP yeu cau moi lenh PHAI ket thuc bang CRLF (\r\n).
// Ham nay tu dong kiem tra va bo sung \r\n neu chua co.
// ===================================================================
bool ClientFTP::sendCommand(const std::string& cmd) {
    if (!isConnected || controlSocket == INVALID_SOCKET) {
        std::cerr << "[TCP Error] Chua ket noi! Goi connectServer() truoc.\n";
        return false;
    }

    // Kiem tra va bo sung CRLF neu chua co
    std::string formattedCmd = cmd;
    if (formattedCmd.size() < 2 ||
        formattedCmd.substr(formattedCmd.size() - 2) != "\r\n") {
        formattedCmd += "\r\n";
    }

    // Gui lenh qua TCP - send() gui het buffer hoac bao loi
    int bytesSent = send(controlSocket,
                         formattedCmd.c_str(),
                         (int)formattedCmd.size(),
                         0);
    if (bytesSent == SOCKET_ERROR) {
        std::cerr << "[TCP Error] Gui lenh that bai!"
                  << " Ma loi: " << WSAGetLastError() << "\n";
        return false;
    }
    return true;
}

// ===================================================================
// readResponse - Doc phan hoi tu Server qua TCP
//
// Buffer 4096 bytes de xu ly cac phan hoi dai nhu ket qua LIST.
// Phan hoi FTP co the co nhieu dong, tat ca deu nam trong mot lan recv()
// trong dieu kien binh thuong (MTU thong thuong du cho mot phan hoi FTP).
//
// Luu y: Ham nay se cap nhat isConnected = false neu Server ngat ket noi.
// ===================================================================
std::string ClientFTP::readResponse() {
    if (!isConnected || controlSocket == INVALID_SOCKET) return "";

    // Bo dem tich luy: doc nhieu lan recv() cho den khi thay \r\n
    // Ly do: TCP co the chia nho mot phan hoi thanh nhieu goi tin nho.
    // Server cua ban dung bo dem tuong tu (recvBuffer trong main.cpp cua server).
    // Neu chi doc 1 lan recv(), co the chi lay duoc nua phan hoi, nua con lai
    // nam o lan recv() tiep theo va bi bo qua -> loi logic rat kho debug.
    std::string accumulated = "";
    char buffer[4096] = {0};

    while (true) {
        int bytesReceived = recv(controlSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived > 0) {
            // Cong don du lieu moi vao bo dem tich luy
            buffer[bytesReceived] = '\0';
            accumulated += std::string(buffer);

            // Kiem tra xem da nhan du mot dong phan hoi chua (co \r\n chua)
            // FTP chuan: moi dong phan hoi PHAI ket thuc bang \r\n
            if (accumulated.find("\r\n") != std::string::npos) {
                return accumulated;
            }
            // Chua thay \r\n -> tiep tuc doc them
        }
        else if (bytesReceived == 0) {
            // Server chu dong dong ket noi (TCP FIN)
            std::cout << "[TCP] Server da dong ket noi.\n";
            isConnected = false;
            return accumulated; // Tra ve nhung gi da tich luy duoc (neu co)
        }
        else {
            // Loi I/O mang
            std::cerr << "[TCP Error] Loi khi doc phan hoi!"
                      << " Ma loi: " << WSAGetLastError() << "\n";
            isConnected = false;
            return "";
        }
    }
}


// ===================================================================
// getReplyCode - Trich xuat ma Reply Code 3 chu so
//
// Moi phan hoi FTP deu bat dau bang 3 chu so theo chuan RFC 959.
// Vi du:
//   "220 Service ready"              -> 220
//   "150 Opening data connection"    -> 150
//   "550 File not found"             -> 550
// ===================================================================
int ClientFTP::getReplyCode(const std::string& response) {
    if (response.size() < 3) return -1;

    try {
        return std::stoi(response.substr(0, 3));
    } catch (...) {
        // Phan hoi khong bat dau bang so (bat thuong)
        return -1;
    }
}

// ===================================================================
// parsePassiveResponse - Phan tich phan hoi PASV (Ma 227)
//
// Dinh dang phan hoi PASV theo RFC 959:
//   "227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)"
//
// Giai thich cong thuc tinh port:
//   Port duoc ma hoa thanh 2 byte: byte cao (p1) va byte thap (p2)
//   dataPort = p1 * 256 + p2
//   Vi du: p1=19, p2=136 -> port = 19*256 + 136 = 4864 + 136 = 5000
//
// Su dung std::regex de dam bao phan tich chinh xac va ben vung
// du Server co them van ban mo ta khac nhau.
// ===================================================================
bool ClientFTP::parsePassiveResponse(const std::string& response,
                                      std::string& dataIP,
                                      int& dataPort) {
    // Regex bat 6 nhom so trong dau ngoac: (h1,h2,h3,h4,p1,p2)
    // R"(...)" la raw string literal de tranh phai escape backslash
    std::regex pasvPattern(R"(\((\d{1,3}),(\d{1,3}),(\d{1,3}),(\d{1,3}),(\d{1,3}),(\d{1,3})\))");
    std::smatch matchResult;

    if (!std::regex_search(response, matchResult, pasvPattern)) {
        std::cerr << "[PASV Error] Khong the phan tich phan hoi PASV:\n";
        std::cerr << "             " << response << "\n";
        std::cerr << "             Kiem tra Server co tra ve dinh dang chuan khong.\n";
        return false;
    }

    // Ghep 4 octet IP thanh chuoi dang "h1.h2.h3.h4"
    dataIP = matchResult[1].str() + "."
           + matchResult[2].str() + "."
           + matchResult[3].str() + "."
           + matchResult[4].str();

    // Tinh port theo cong thuc: port = p1 * 256 + p2
    int p1 = std::stoi(matchResult[5].str());
    int p2 = std::stoi(matchResult[6].str());
    dataPort = p1 * 256 + p2;

    return true;
}

// ===================================================================
// buildPortCommand - Xay dung lenh PORT cho Active Mode
//
// Trong Active Mode, Client thong bao cho Server biet IP va Port
// ma Client dang lang nghe. Server se CHU DONG ket noi nguoc lai.
//
// Cac buoc:
//   1. Goi getsockname() tren controlSocket de lay IP cuc bo thuc te
//      (tranh hardcode, tu dong thich nghi voi nhieu card mang)
//   2. Chuyen dia chi nhi phan sang chuoi dung inet_ntop
//   3. Thay dau '.' bang ',' trong IP theo yeu cau dinh dang FTP
//   4. Tinh p1 = port / 256, p2 = port % 256
//   5. Ghep thanh chuoi "PORT h1,h2,h3,h4,p1,p2"
//
// localPort: port ma Client se mo de don nhan ket noi UDP tu Server
//            (truyen 0 neu muon de ham tu chon port ngu nhien)
// ===================================================================
std::string ClientFTP::buildPortCommand(int localPort) {
    if (controlSocket == INVALID_SOCKET) {
        std::cerr << "[PORT Error] Socket chua duoc khoi tao!\n";
        return "";
    }

    // Lay dia chi cuc bo cua socket hien tai
    sockaddr_in localAddr;
    ZeroMemory(&localAddr, sizeof(localAddr));
    int addrLen = sizeof(localAddr);

    if (getsockname(controlSocket, (sockaddr*)&localAddr, &addrLen) == SOCKET_ERROR) {
        std::cerr << "[PORT Error] Khong lay duoc dia chi cuc bo!"
                  << " Ma loi: " << WSAGetLastError() << "\n";
        return "";
    }

    // Chuyen IP nhi phan sang chuoi dang "192.168.1.10"
    char ipBuffer[INET_ADDRSTRLEN] = {0};
    if (inet_ntop(AF_INET, &localAddr.sin_addr, ipBuffer, INET_ADDRSTRLEN) == nullptr) {
        std::cerr << "[PORT Error] Khong chuyen duoc IP sang chuoi!\n";
        return "";
    }

    // Thay dau cham '.' bang dau phay ',' theo dinh dang lenh PORT cua FTP
    std::string ipStr = ipBuffer;
    for (char& c : ipStr) {
        if (c == '.') c = ',';
    }

    // Phan ra 2 byte port theo thu tu byte cao (p1) va byte thap (p2)
    int p1 = (localPort >> 8) & 0xFF;   // byte cao: port / 256
    int p2 =  localPort       & 0xFF;   // byte thap: port % 256

    return "PORT " + ipStr + "," + std::to_string(p1) + "," + std::to_string(p2);
}

// ===================================================================
// disconnect - Dong socket TCP va dat lai trang thai
// ===================================================================
void ClientFTP::disconnect() {
    if (controlSocket != INVALID_SOCKET) {
        closesocket(controlSocket);
        controlSocket = INVALID_SOCKET;
    }
    isConnected = false;
}