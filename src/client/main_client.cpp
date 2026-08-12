// ===================================================================
// main.cpp - Hybrid FTP Client
// Phien ban: 5.0 - Tich hop kiem tra toan ven bang MD5 Hash
//
// Luong kiem tra toan ven:
//   STOR (Upload):
//     1. Bam file truoc khi gui: calculate_file_hash(file) -> localHash
//     2. Gui file: rdt_send_file_stream(dataSock, dataDest, file)
//     3. Gui hash : rdt_send_buffer(dataSock, dataDest, localHash)
//     4. Nhan phan hoi 226 qua control channel
//
//   RETR (Download):
//     1. Nhan file : rdt_receive_file_stream(dataSock, file)
//     2. Bam file vua nhan: calculate_file_hash(file) -> localHash
//     3. Nhan hash tu Server: rdt_receive_buffer(dataSock) -> serverHash
//     4. So sanh localHash == serverHash -> bao nguyen ven hoac canh bao
//     5. Nhan phan hoi 226 qua control channel
//
// Tat ca gui/nhan deu qua ham RDT, khong dung send()/recv() thuan.
// ===================================================================

#include <iostream>
#include <string>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <filesystem>

#include "../../include/client/client_ftp.h"
#include "../../include/client/client_cli.h"
#include "../../include/core/rdt.h"
#include "../../include/core/crypto_hash.h"   // calculate_file_hash()

// ===================================================================
// buildCmd - Tao format lenh FTP de gui qua rdt_send_buffer
//
// Dau ra: vector<char> chua chuoi "CMD arg\r\n"
// Vi du:
//   buildCmd("USER", "admin") -> "USER admin\r\n" dang bytes
//   buildCmd("PASV", "")      -> "PASV\r\n"       dang bytes
// ===================================================================
static std::vector<char> buildCmd(const std::string& cmd, const std::string& arg) {
    std::string msg = arg.empty() ? cmd : (cmd + " " + arg);
    msg += "\r\n";
    return std::vector<char>(msg.begin(), msg.end());
}

// ===================================================================
// parseResp - Chuyen du lieu tu rdt_receive_buffer thanh chuoi phan hoi
//
// rdt_receive_buffer tra ve vector<char>, parseResp chuyen sang string
// de xu ly ma phan hoi, in ra man hinh, so sanh, v.v.
// ===================================================================
static std::string parseResp(const std::vector<char>& data) {
    if (data.empty()) return "";
    return std::string(data.begin(), data.end());
}

// ===================================================================
// sendCmd - Gui lenh FTP len Server qua RDT
//
// Buoc 1: buildCmd -> "CMD arg\r\n" -> vector<char>
// Buoc 2: rdt_send_buffer -> gui qua UDP (ACK + Timeout + Retransmit)
// ===================================================================
static bool sendCmd(SOCKET sock, const sockaddr_in& dest,
                    const std::string& cmd, const std::string& arg = "") {
    std::vector<char> payload = buildCmd(cmd, arg);
    if (!rdt_send_buffer(sock, dest, payload)) {
        std::cerr << "  [RDT Error] Gui lenh that bai: " << cmd << "\n";
        return false;
    }
    return true;
}

// ===================================================================
// readResp - Nhan phan hoi tu Server qua RDT
//
// Buoc 1: rdt_receive_buffer -> nhan bytes tu Server
// Buoc 2: parseResp          -> chuyen sang std::string
// ===================================================================
static std::string readResp(SOCKET sock) {
    return parseResp(rdt_receive_buffer(sock));
}

// ===================================================================
// handleReplyCode - May trang thai xu ly ma phan hoi FTP (RFC 959)
// ===================================================================
static void handleReplyCode(int code, const std::string& response) {
    if (code >= 100 && code < 200) {
        std::cout << "  [>>] Ma " << code << ": Server dang mo Data Channel...\n";
    } else if (code >= 200 && code < 300) {
        switch (code) {
        case 220: std::cout << "  [OK] Ma 220: Server san sang.\n";         break;
        case 221: std::cout << "  [OK] Ma 221: Server dong phien.\n";        break;
        case 226: std::cout << "  [OK] Ma 226: Truyen du lieu hoan tat.\n";  break;
        case 227: /* xu ly trong switch chinh */                              break;
        case 230: std::cout << "  [OK] Ma 230: Dang nhap thanh cong!\n";    break;
        case 250: std::cout << "  [OK] Ma 250: Thao tac hoan thanh.\n";     break;
        default:  std::cout << "  [OK] Ma " << code << ": Thanh cong.\n";   break;
        }
    } else if (code >= 300 && code < 400) {
        if      (code == 331) std::cout << "  [..] Ma 331: Can mat khau -> PASS <password>\n";
        else if (code == 350) std::cout << "  [..] Ma 350: San sang doi ten -> RNTO <ten_moi>\n";
        else                  std::cout << "  [..] Ma " << code << ": Can buoc tiep theo.\n";
    } else if (code >= 400 && code < 500) {
        std::cerr << "  [!!] Ma " << code << " (Loi tam thoi): " << response;
    } else if (code >= 500) {
        std::cerr << "  [XX] Ma " << code << " (Loi): " << response;
        if      (code == 530) std::cerr << "       Chua dang nhap. Dung USER/PASS.\n";
        else if (code == 550) std::cerr << "       File khong ton tai hoac khong co quyen.\n";
        else if (code == 502) std::cerr << "       Lenh chua duoc ho tro.\n";
    } else if (code == -1) {
        std::cerr << "  [??] Phan hoi khong xac dinh: " << response << "\n";
    }
}

// ===================================================================
// main
// ===================================================================
int main() {
    // BUOC 1: Banner
    std::cout << "\n";
    std::cout << "  +==============================================================+\n";
    std::cout << "  |      HYBRID FTP CLIENT  -  Giao tiep thong nhat qua RDT     |\n";
    std::cout << "  |      Phien ban: 5.0 | Kiem tra toan ven bang MD5 Hash       |\n";
    std::cout << "  +==============================================================+\n\n";

    std::filesystem::create_directories("client_data");
    std::cout << "  [System] Thu muc lam viec cua Client: ./client_data\n\n";

    // BUOC 2: Nhap IP / Port Server
    std::string serverIP;
    int         serverPort = 2121;
    std::string portStr;

    std::cout << "  Nhap IP Server  [mac dinh: 127.0.0.1] : ";
    std::getline(std::cin, serverIP);
    if (serverIP.empty()) serverIP = "127.0.0.1";

    std::cout << "  Nhap Port       [mac dinh: 2121]      : ";
    std::getline(std::cin, portStr);
    if (!portStr.empty()) {
        try {
            serverPort = std::stoi(portStr);
            if (serverPort <= 0 || serverPort > 65535) serverPort = 2121;
        } catch (...) { serverPort = 2121; }
    }
    std::cout << "\n";

    // BUOC 3: Khoi tao Winsock (1 lan duy nhat)
    if (!rdt_init()) {
        std::cerr << "  [Loi] Khoi tao Winsock that bai!\n";
        return 1;
    }

    // BUOC 4: Tao UDP socket va luu dia chi Server
    ClientFTP ftpClient;
    std::cout << "  Dang khoi tao Control Channel toi " << serverIP << ":" << serverPort << " ...\n";

    if (!ftpClient.connectServer(serverIP, serverPort)) {
        std::cerr << "\n  [Loi] Khong the ket noi toi Server.\n";
        rdt_cleanup();
        return 1;
    }
    ClientCLI::printStatus(serverIP, serverPort, true);

    SOCKET      ctrlSock = ftpClient.getSocket();
    sockaddr_in ctrlDest = ftpClient.getServerAddr();

    // =========================================================================
    // BUOC 5: GUI GOI TIN KICH HOAT SERVER (Bắt buộc cho UDP)
    // =========================================================================
    const char* initPing = "INIT\r\n";
    // Dùng trực tiếp send() vì UDP socket đã được connect() trong connectServer
    int sendRes = send(ctrlSock, initPing, (int)strlen(initPing), 0);
    if (sendRes == SOCKET_ERROR) {
        // Fallback sang sendto nếu send bị từ chối
        sendto(ctrlSock, initPing, (int)strlen(initPing), 0, (sockaddr*)&ctrlDest, sizeof(ctrlDest));
    }
    std::cout << "  [UDP] Da gui goi tin KICH HOAT Server (" << strlen(initPing) << " bytes)...\n";

    // Doc thong diep chao (Ma 220) qua rdt_receive_buffer
    std::vector<char> welcomeData = rdt_receive_buffer(ctrlSock, &ctrlDest);
    std::string welcomeMsg = parseResp(welcomeData);
    if (!welcomeMsg.empty()) {
        std::cout << "  Server: " << welcomeMsg;
        handleReplyCode(ftpClient.getReplyCode(welcomeMsg), welcomeMsg);
        std::cout << "  [System] Da chuyen Control Channel sang Port moi: " << ntohs(ctrlDest.sin_port) << "\n";
    }
    ClientCLI::printHelp();

    // BUOC 6: Vong lap CLI
    int         dataChannelPort = 0;
    sockaddr_in dataDest;
    ZeroMemory(&dataDest, sizeof(dataDest));

    bool isPassive = true; 
    SOCKET activeDataSock = INVALID_SOCKET;

    std::string lastCmd;
    std::string lastArg;
    std::string inputLine;

    while (true) {
        if (dataChannelPort > 0)
            std::cout << "ftp [DataReady:" << dataChannelPort << "]> ";
        else
            std::cout << "ftp> ";

        if (!std::getline(std::cin, inputLine)) break;
        if (inputLine.empty()) continue;

        std::string cmd, arg;
        if (!ClientCLI::parseAndValidate(inputLine, cmd, arg)) continue;

        // --- QUIT ---
        if (cmd == "QUIT") {
            std::cout << "  Dang ngat ket noi...\n";
            sendCmd(ctrlSock, ctrlDest, "QUIT");
            std::string r = readResp(ctrlSock);
            if (!r.empty()) std::cout << "  Server: " << r;
            break;
        }

        // --- HELP noi bo ---
        if (cmd == "HELP" && arg.empty()) {
            ClientCLI::printHelp();
            continue;
        }

        // --- THÊM KHỐI XỬ LÝ PORT TỰ ĐỘNG NÀY ---
        if (cmd == "PORT") {
            if (arg.empty()) {
                std::cerr << "  [Loi] Lenh PORT can tham so. Vi du: PORT 192,168,1,5,195,149\n";
                continue;
            }

            int h1, h2, h3, h4, p1, p2;
            // Dùng sscanf để tách 6 số nguyên phân cách bằng dấu phẩy
            if (sscanf(arg.c_str(), "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2) != 6) {
                std::cerr << "  [Loi] Sai cu phap PORT. Yeu cau: h1,h2,h3,h4,p1,p2\n";
                continue;
            }

            // Tính toán số Port từ 2 tham số p1, p2
            int port = (p1 * 256) + p2;
            std::string ip = std::to_string(h1) + "." + std::to_string(h2) + "." + std::to_string(h3) + "." + std::to_string(h4);

            if (activeDataSock != INVALID_SOCKET) {
                closesocket(activeDataSock);
            }
            
            activeDataSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (activeDataSock == INVALID_SOCKET) {
                std::cerr << "  [Loi] Khong the tao Data Socket.\n";
                continue;
            }

            sockaddr_in localAddr;
            ZeroMemory(&localAddr, sizeof(localAddr));
            localAddr.sin_family = AF_INET;
            
            // INADDR_ANY giúp Client lắng nghe trên tất cả các card mạng (bao gồm cả LAN và Wi-Fi)
            localAddr.sin_addr.s_addr = INADDR_ANY; 
            localAddr.sin_port = htons(port);
            
            // Bind (ràng buộc) socket vào đúng cái Port mà người dùng vừa nhập
            if (bind(activeDataSock, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
                std::cerr << "  [Loi] Khong the bind vao port " << port << ". Vui long chon port khac (VD: p1 > 10).\n";
                closesocket(activeDataSock);
                activeDataSock = INVALID_SOCKET;
                continue;
            }
            
            isPassive = false; 
            dataChannelPort = port;
            std::cout << "  [Active] Client da mo Data Socket, lang nghe tai IP: " << ip << ", Port: " << port << "\n";
        }

        // --- Gui lenh qua rdt_send_buffer ---
        if (!sendCmd(ctrlSock, ctrlDest, cmd, arg)) {
            std::cerr << "  [Loi] Gui lenh that bai.\n";
            break;
        }
        lastCmd = cmd;
        lastArg = arg;

        // --- Doc phan hoi qua rdt_receive_buffer ---
        std::string response = readResp(ctrlSock);
        if (response.empty()) {
            std::cerr << "  [Loi] Khong nhan duoc phan hoi.\n";
            break;
        }
        std::cout << "  Server: " << response<< std::flush;

        int code = ftpClient.getReplyCode(response);
        handleReplyCode(code, response);

        switch (code) {

        // ===================================================================
        // Ma 150: Server san sang -> bat dau truyen file qua Data Channel
        // ===================================================================
        case 150: {
            if (dataChannelPort == 0 && isPassive) {
                std::cerr << "  [Loi] Chua co Data Channel! Gui PASV hoac PORT truoc.\n";
                break;
            }

            SOCKET dataSock = INVALID_SOCKET;

            if (isPassive) {
                // PASSIVE MODE: Tạo mới socket và Bind vào port 0
                dataSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                if (dataSock == INVALID_SOCKET) {
                    std::cerr << "  [Loi] Khong tao duoc Data socket.\n";
                    dataChannelPort = 0;
                    break;
                }

                sockaddr_in localData;
                ZeroMemory(&localData, sizeof(localData));
                localData.sin_family = AF_INET;
                localData.sin_addr.s_addr = INADDR_ANY;
                localData.sin_port = 0;
                
                if (bind(dataSock, (sockaddr*)&localData, sizeof(localData)) == SOCKET_ERROR) {
                    std::cerr << "  [Loi] Bind Data socket that bai.\n";
                    closesocket(dataSock);
                    dataChannelPort = 0;
                    break;
                }

                // Gửi tín hiệu kích hoạt cho Server
                char readyByte = '1';
                sendto(dataSock, &readyByte, 1, 0, (sockaddr*)&dataDest, sizeof(dataDest));
            } else {
                // ACTIVE MODE: Tái sử dụng socket đã mở lúc gõ lệnh PORT
                dataSock = activeDataSock;
                std::cout << "  [Active] Cho Server ket noi va truyen du lieu...\n";
            }

            std::string localFilePath = "client_data/" + lastArg;

            if (lastCmd == "RETR") {
                // Kiem tra co the tao file dau ra khong
                {
                    std::ofstream testOut(localFilePath, std::ios::binary); // <-- Dùng localFilePath
                    if (!testOut.is_open()) {
                        std::cerr << "  [Loi] Khong tao duoc file: '" << localFilePath << "'\n";
                        closesocket(dataSock);
                        break;
                    }
                    testOut.close();
                    std::remove(localFilePath.c_str());
                }

                std::cout << "  [RDT] Bat dau nhan file: '" << localFilePath << "'\n";

                // Buoc 1: Nhan file qua rdt (lưu vào client_data/)
                bool recvOK = rdt_receive_file_stream(dataSock, localFilePath); // <-- Dùng localFilePath

                if (recvOK) {
                    std::cout << "  [RDT] Nhan file hoan tat: " << localFilePath << "\n";

                    // Buoc 2: Bam file vua nhan bang MD5
                    std::string localHash = calculate_file_hash(localFilePath);
                    if (localHash.empty()) {
                        std::cerr << "  [Hash] Khong the tinh hash cua file vua nhan!\n";
                    } else {
                        std::cout << "  [Hash] MD5 local : " << localHash << "\n";
                    }

                    // Buoc 3: Nhan chuoi hash tu Server qua rdt_receive_buffer
                    std::vector<char> hashData = rdt_receive_buffer(dataSock);
                    std::string serverHash = parseResp(hashData);

                    // Loai bo \r\n neu co
                    while (!serverHash.empty() &&
                           (serverHash.back() == '\r' || serverHash.back() == '\n'))
                        serverHash.pop_back();

                    if (serverHash.empty()) {
                        std::cerr << "  [Hash] Khong nhan duoc hash tu Server!\n";
                    } else {
                        std::cout << "  [Hash] MD5 server: " << serverHash << "\n";
                    }

                    // Buoc 4: So sanh hai chuoi MD5
                    if (!localHash.empty() && !serverHash.empty()) {
                        if (localHash == serverHash) {
                            std::cout << "  [Hash] KHOP! File nguyen ven, khong bi loi.\n";
                        } else {
                            std::cerr << "  [Hash] KHONG KHOP! File bi hu hong khi truyen!\n";
                        }
                    }

                    // Buoc 5: Doc ma 226 tu Control Channel
                    std::string done = readResp(ctrlSock);
                    if (!done.empty()) {
                        std::cout << "  Server: " << done;
                        handleReplyCode(ftpClient.getReplyCode(done), done);
                    }
                } else {
                    std::cerr << "  [RDT] Nhan file that bai.\n";
                }
            }

            // -----------------------------------------------------------
            // STOR: Upload file len Server
            //
            // Luong:
            //   1. calculate_file_hash(file) -> tinh MD5 truoc khi gui
            //   2. rdt_send_file_stream -> gui file nhi phan
            //   3. rdt_send_buffer -> gui chuoi MD5 de Server doi chieu
            //   4. readResp(ctrlSock) -> nhan ma 226 tu Control Channel
            // -----------------------------------------------------------
            else if (lastCmd == "STOR") {
                if (!isPassive) {
                    std::cout << "  [Active] Dang cho Server ping de xac dinh huong di...\n";
                    
                    // Sử dụng rdt_receive_buffer và truyền dataDest vào để hàm tự cập nhật IP/Port của Server
                    std::vector<char> pingMsg = rdt_receive_buffer(dataSock, &dataDest); 
                    
                    if (pingMsg.empty()) {
                        std::cerr << "  [Loi] Khong nhan duoc Ping tu Server. Huy Upload.\n";
                        closesocket(dataSock);
                        dataChannelPort = 0;
                        break;
                    }
                    std::cout << "  [Active] Da khoa muc tieu Server Data Port: " << ntohs(dataDest.sin_port) << "\n";
                }
                // Kiem tra file ton tai trong client_data/
                uint64_t fileSize = 0;
                {
                    std::ifstream chk(localFilePath, std::ios::binary | std::ios::ate); // <-- Dùng localFilePath
                    if (!chk.is_open()) {
                        std::cerr << "  [Loi] Khong mo duoc file: '" << localFilePath << "' (Hay de file vao folder client_data!)\n";
                        closesocket(dataSock);
                        dataChannelPort = 0;
                        break;
                    }
                    fileSize = (uint64_t)chk.tellg();
                }

                // Buoc 1: Bam file truoc khi gui
                std::string localHash = calculate_file_hash(localFilePath); // <-- Dùng localFilePath
                if (localHash.empty()) {
                    std::cerr << "  [Hash] Khong the tinh hash! Huy upload.\n";
                    closesocket(dataSock);
                    dataChannelPort = 0;
                    break;
                }
                std::cout << "  [Hash] MD5 local : " << localHash << "\n";

                std::cout << "  [RDT] Upload: '" << localFilePath << "' (" << fileSize << " bytes)\n";

                // Buoc 2: Gui file qua rdt_send_file_stream
                bool sendOK = rdt_send_file_stream(dataSock, dataDest, localFilePath); // <-- Dùng localFilePath

                if (sendOK) {
                    std::cout << "  [RDT] Gui file hoan tat.\n";

                    Sleep(100);

                    // Buoc 3: Gui chuoi MD5 de Server doi chieu voi file vua nhan
                    // Them \r\n de Server de xu ly cu phap
                    std::string hashMsg = localHash + "\r\n";
                    std::vector<char> hashPayload(hashMsg.begin(), hashMsg.end());
                    if (rdt_send_buffer(dataSock, dataDest, hashPayload)) {
                        std::cout << "  [Hash] Da gui MD5 hash den Server.\n";
                    } else {
                        std::cerr << "  [Hash] Gui hash that bai!\n";
                    }

                    // Buoc 4: Doc ma 226 tu Control Channel
                    std::string done = readResp(ctrlSock);
                    if (!done.empty()) {
                        std::cout << "  Server: " << done;
                        handleReplyCode(ftpClient.getReplyCode(done), done);
                    }
                } else {
                    std::cerr << "  [RDT] Upload that bai.\n";
                }
            }

            // -----------------------------------------------------------
            // LIST / NLST: Nhan danh sach thu muc dang van ban
            // -----------------------------------------------------------
            else if (lastCmd == "LIST" || lastCmd == "NLST") {
                std::cout << "  [RDT] Dang nhan danh sach thu muc...\n";

                std::vector<char> listData = rdt_receive_buffer(dataSock);

                if (!listData.empty()) {
                    std::cout << "\n  --- Noi dung thu muc ---\n";
                    std::cout << parseResp(listData);
                    std::cout << "  ------------------------\n\n";
                }

                std::string done = readResp(ctrlSock);
                if (!done.empty()) std::cout << "  Server: " << done;
            }

            if (isPassive) {
                closesocket(dataSock);
            } else {
                closesocket(activeDataSock);
                activeDataSock = INVALID_SOCKET;
            }
            dataChannelPort = 0;
            break;
        }

        // Ma 226: Truyen hoan tat
        case 226: {
            std::cout << "  [Done] Truyen du lieu hoan tat.\n";
            dataChannelPort = 0;
            break;
        }

        // Ma 227: PASV OK - cap nhat dia chi Data Channel cua Server
        case 227: {
            isPassive = true; // --- THÊM DÒNG NÀY ---
            if (activeDataSock != INVALID_SOCKET) { // --- THÊM ĐOẠN NÀY ---
                closesocket(activeDataSock);
                activeDataSock = INVALID_SOCKET;
            }

            std::string psvIP;
            int         psvPort = 0;
            if (ftpClient.parsePassiveResponse(response, psvIP, psvPort)) {
                dataChannelPort = psvPort;

                ZeroMemory(&dataDest, sizeof(dataDest));
                dataDest.sin_family = AF_INET;
                dataDest.sin_port   = htons((u_short)psvPort);
                dataDest.sin_addr.s_addr = inet_addr(psvIP.c_str());

                std::cout << "  [PASV] Data Channel: " << psvIP << ":" << psvPort << "\n";
                std::cout << "  [PASV] San sang! Gui RETR <file>, STOR <file>, hoac LIST.\n";
            }
            break;
        }

        // Ma 230: Dang nhap thanh cong
        case 230: {
            std::cout << "  [Auth] Chao mung! Phien da bat dau.\n";
            ClientCLI::printStatus(serverIP, serverPort, true);
            break;
        }

        default:
            break;
        }
    }

    // BUOC 7: Don dep
    ftpClient.disconnect();
    ClientCLI::printStatus(serverIP, serverPort, false);
    std::cout << "  Phien FTP ket thuc. Tam biet!\n\n";
    rdt_cleanup();
    return 0;
}