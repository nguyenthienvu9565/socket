#include <iostream>
#include <string>
#include <fstream>      // std::ifstream/ofstream + std::ios::binary
#include <cstdint>      // uint64_t

#include "../../include/client/client_ftp.h"
#include "../../include/client/client_cli.h"

// ===================================================================
// computeChecksum - Tinh checksum cua file local de kiem tra toan ven
//
// Dung cung thuat toan voi Server (transfer_handler.cpp):
//   accumulatedHash = (accumulatedHash + byte) % 1000000007
// Neu checksum local == checksum Server tra ve trong ma 226 thi
// file da duoc truyen nguyen ven, khong bi loi hay mat goi tin.
// ===================================================================
static long long computeChecksum(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) return -1;

    long long hash = 0;
    char byte;
    while (file.get(byte)) {
        // Ep kieu sang unsigned char truoc khi cong vao hash
        // de tranh so am lam sai ket qua tinh toan
        hash = (hash + (unsigned char)byte) % 1000000007;
    }
    return hash;
}

// ===================================================================
// parseChecksumFrom226 - Trich xuat gia tri checksum tu phan hoi 226
//
// Server gui ve dang: "226 Transfer complete. Checksum: 123456"
// Ham nay tim chu "Checksum: " va lay so phia sau no.
// ===================================================================
static long long parseChecksumFrom226(const std::string& response) {
    // Tim vi tri cua chu "Checksum: " trong chuoi phan hoi
    std::string keyword = "Checksum: ";
    size_t pos = response.find(keyword);
    if (pos == std::string::npos) return -1; // Khong co checksum trong phan hoi

    // Lay phan so phia sau chu "Checksum: "
    std::string numStr = response.substr(pos + keyword.size());
    try {
        return std::stoll(numStr);
    } catch (...) {
        return -1;
    }
}

// ===================================================================
//  RDT STUB FUNCTIONS - Placeholder cho module truyen file tin cay
//
//  Cac ham nay la cac stub (ban mau) doi cho den khi module RDT
//  cua Thanh vien 3 duoc tich hop vao du an.
//
//  De tich hop:
//    1. Xoa cac ham stub duoi day
//    2. #include header file cua module RDT
//    3. Dam bao chu ky ham khop voi dinh nghia trong module RDT
//
//  Yeu cau ky thuat voi module RDT that su:
//    - Dung SOCK_DGRAM (UDP) de truyen du lieu
//    - Phai co Sequence Number (so thu tu goi tin)
//    - Phai co Co che ACK + Timeout + Retransmit
//    - Doc/ghi file PHAI dung co std::ios::binary (file nhi phan)
// ===================================================================

// rdt_receive_file: Nhan file tu Server ve va luu ra dia
// saveFilePath : Duong dan luu file (VD: "anh.jpg", "D:/Downloads/video.mp4")
// serverIP     : IP cua Server (de loc goi tin den dung noi)
// dataPort     : UDP Port ma Server dang gui du lieu
// fileSize     : Kich thuoc file du kien (0 = khong biet truoc)
// Tra ve       : true neu nhan thanh cong, false neu that bai
static bool rdt_receive_file(const std::string& saveFilePath,
                              const std::string& serverIP,
                              int dataPort,
                              uint64_t fileSize) {
    // [STUB] Can duoc thay bang implementation RDT thuc te
    std::cout << "\n  [RDT Stub] rdt_receive_file() chua duoc implement.\n";
    std::cout << "             - Luu file: " << saveFilePath << "\n";
    std::cout << "             - Server UDP: " << serverIP << ":" << dataPort << "\n";
    if (fileSize > 0)
        std::cout << "             - Kich thuoc: " << fileSize << " bytes\n";
    std::cout << "             Tich hop module RDT de dung tinh nang nay.\n\n";
    return false;
}

// rdt_send_file: Doc file nhi phan va gui len Server qua UDP
// sourceFilePath : Duong dan file can upload tren may Client
// serverIP       : IP cua Server nhan du lieu
// dataPort       : UDP Port ma Server dang lang nghe
// Tra ve         : true neu gui thanh cong, false neu that bai
static bool rdt_send_file(const std::string& sourceFilePath,
                           const std::string& serverIP,
                           int dataPort) {
    // [STUB] Can duoc thay bang implementation RDT thuc te
    std::cout << "\n  [RDT Stub] rdt_send_file() chua duoc implement.\n";
    std::cout << "             - File nguon: " << sourceFilePath << "\n";
    std::cout << "             - Server UDP: " << serverIP << ":" << dataPort << "\n";
    std::cout << "             Tich hop module RDT de dung tinh nang nay.\n\n";
    return false;
}

// rdt_receive_listing: Nhan chuoi van ban danh sach thu muc qua UDP
// serverIP  : IP cua Server gui du lieu
// dataPort  : UDP Port ma Server dang gui danh sach
// Tra ve    : Chuoi van ban chua danh sach, hoac chuoi rong neu loi
static std::string rdt_receive_listing(const std::string& serverIP, int dataPort) {
    // [STUB] Can duoc thay bang implementation RDT thuc te
    std::cout << "\n  [RDT Stub] rdt_receive_listing() chua duoc implement.\n";
    std::cout << "             - Server UDP: " << serverIP << ":" << dataPort << "\n\n";
    return "";
}

// ===================================================================
// handleReplyCode - May Trang Thai xu ly ma phan hoi FTP (Muc 2.3)
//
// Cac nhom ma theo chuan RFC 959:
//   1xx - Positive Preliminary Reply: Dang xu ly, can doi tiep
//   2xx - Positive Completion Reply : Lenh hoan thanh thanh cong
//   3xx - Positive Intermediate     : Can them thong tin trung gian
//   4xx - Transient Negative Reply  : Loi tam thoi (co the thu lai)
//   5xx - Permanent Negative Reply  : Loi vinh vien (khong thu lai)
// ===================================================================
static void handleReplyCode(int code, const std::string& response) {
    if (code >= 100 && code < 200) {
        // Nhom 1xx: Server chap nhan va dang bat dau xu ly
        // Ma pho bien: 125 (da co ket noi du lieu), 150 (dang mo ket noi)
        std::cout << "  [>>] Ma " << code << ": Server dang mo ket noi du lieu...\n";
    }
    else if (code >= 200 && code < 300) {
        // Nhom 2xx: Lenh thuc hien thanh cong
        // Ma pho bien: 200 (OK), 220 (san sang), 221 (bye), 226 (xong), 230 (login ok), 250 (ok)
        switch (code) {
        case 220: std::cout << "  [OK] Ma 220: Server san sang phuc vu.\n";  break;
        case 221: std::cout << "  [OK] Ma 221: Server da dong phien.\n";      break;
        case 226: std::cout << "  [OK] Ma 226: Truyen du lieu hoan tat.\n";   break;
        case 227: /* Xu ly o ben duoi trong switch(code) chinh */             break;
        case 230: std::cout << "  [OK] Ma 230: Dang nhap thanh cong!\n";     break;
        case 250: std::cout << "  [OK] Ma 250: Thao tac file hoan thanh.\n"; break;
        default:  std::cout << "  [OK] Ma " << code << ": Thanh cong.\n";    break;
        }
    }
    else if (code >= 300 && code < 400) {
        // Nhom 3xx: Can them buoc trung gian
        // Ma pho bien: 331 (can password), 350 (can RNTO sau RNFR)
        if (code == 331) {
            std::cout << "  [..] Ma 331: Ten dang nhap hop le. Hay nhap mat khau:\n";
            std::cout << "              Go lenh: PASS <mat_khau>\n";
        } else if (code == 350) {
            std::cout << "  [..] Ma 350: San sang doi ten. Hay gui lenh RNTO <ten_moi>.\n";
        } else {
            std::cout << "  [..] Ma " << code << ": Can buoc trung gian tiep theo.\n";
        }
    }
    else if (code >= 400 && code < 500) {
        // Nhom 4xx: Loi tam thoi - co the thu lai sau
        // Ma pho bien: 421 (Service unavailable), 425 (khong mo duoc data channel), 450 (file busy)
        std::cerr << "  [!!] Ma " << code << " (Loi tam thoi): " << response;
        std::cerr << "       Server hien tai ban hoac file dang bi khoa. Thu lai sau.\n";
    }
    else if (code >= 500) {
        // Nhom 5xx: Loi vinh vien - khong thu lai
        // Ma pho bien: 530 (chua login), 550 (file khong ton tai/quyen), 502 (lenh chua ho tro)
        std::cerr << "  [XX] Ma " << code << " (Loi): " << response;
        if (code == 530) {
            std::cerr << "       Chua dang nhap hoac phien het han. Hay dung USER/PASS.\n";
        } else if (code == 550) {
            std::cerr << "       File/thu muc khong ton tai hoac khong co quyen truy cap.\n";
        } else if (code == 502) {
            std::cerr << "       Lenh nay Server chua ho tro.\n";
        }
    }
    else if (code == -1) {
        // Phan hoi khong ro rang (khong dung dinh dang FTP)
        std::cerr << "  [??] Phan hoi khong xac dinh: " << response << "\n";
    }
}

// ===================================================================
// main - Diem khoi dau chuong trinh Hybrid FTP Client
//
// Luong hoat dong tong quat:
//   1. Hien thi banner va cho nguoi dung nhap IP/Port Server
//   2. Ket noi TCP toi Control Channel
//   3. Doc thong diep chao (Ma 220) tu Server
//   4. Vong lap CLI: nhan lenh -> gui TCP -> xu ly phan hoi -> [UDP neu can]
//   5. Ket thuc: gui QUIT -> dong socket
//
// Tich hop Data Channel (UDP):
//   - Nguoi dung PHAI gui PASV hoac PORT truoc lenh RETR/STOR/LIST
//   - Khi nhan ma 227 (PASV OK): luu IP/Port data channel
//   - Khi nhan ma 150 (Opening data): kich hoat luong RDT tuong ung
// ===================================================================
int main() {
    // ---------------------------------------------------------------
    // BUOC 1: Hien thi banner chao
    // ---------------------------------------------------------------
    std::cout << "\n";
    std::cout << "  +==============================================================+\n";
    std::cout << "  |      HYBRID FTP CLIENT  (TCP Control  +  UDP Data RDT)      |\n";
    std::cout << "  |      Phien ban: 2.0 | Ho tro: Active/Passive Mode           |\n";
    std::cout << "  +==============================================================+\n";
    std::cout << "\n";

    // ---------------------------------------------------------------
    // BUOC 2: Nhap thong tin ket noi tu ban phim
    // Khong hardcode IP/Port: cho phep Client ket noi den bat ky Server nao
    // ---------------------------------------------------------------
    std::string serverIP;
    int         serverPort  = 2121;
    std::string portStr;

    std::cout << "  Nhap dia chi IP Server  [mac dinh: 127.0.0.1] : ";
    std::getline(std::cin, serverIP);
    if (serverIP.empty()) serverIP = "127.0.0.1";

    std::cout << "  Nhap Port Control       [mac dinh: 2121]      : ";
    std::getline(std::cin, portStr);
    if (!portStr.empty()) {
        try {
            serverPort = std::stoi(portStr);
            if (serverPort <= 0 || serverPort > 65535) {
                std::cerr << "  [Canh bao] Port khong hop le, dung mac dinh 2121.\n";
                serverPort = 2121;
            }
        } catch (...) {
            std::cerr << "  [Canh bao] Port khong phai so, dung mac dinh 2121.\n";
            serverPort = 2121;
        }
    }
    std::cout << "\n";

    // ---------------------------------------------------------------
    // BUOC 3: Khoi tao doi tuong va ket noi TCP
    // ---------------------------------------------------------------
    ClientFTP ftpClient;
    std::cout << "  Dang ket noi toi " << serverIP << ":" << serverPort << " ...\n";

    if (!ftpClient.connectServer(serverIP, serverPort)) {
        std::cerr << "\n  [Loi] Khong the ket noi toi Server.\n";
        std::cerr << "        - Kiem tra Server co dang chay khong\n";
        std::cerr << "        - Kiem tra IP va Port da nhap dung chua\n";
        std::cerr << "        - Kiem tra Firewall co chan ket noi khong\n\n";
        return 1;
    }

    // Hien thi trang thai ket noi len console
    ClientCLI::printStatus(serverIP, serverPort, true);

    // ---------------------------------------------------------------
    // BUOC 4: Doc thong diep chao mung tu Server (Ma 220)
    // ---------------------------------------------------------------
    std::string welcomeMsg = ftpClient.readResponse();
    if (!welcomeMsg.empty()) {
        std::cout << "  Server: " << welcomeMsg;
        handleReplyCode(ftpClient.getReplyCode(welcomeMsg), welcomeMsg);
    }

    // Hien thi bang huong dan su dung
    ClientCLI::printHelp();

    // ---------------------------------------------------------------
    // BUOC 5: Vong lap CLI chinh
    //
    // Bien trang thai Data Channel:
    //   dataChannelIP   : IP cua Server se gui/nhan file qua UDP
    //   dataChannelPort : Port UDP tuong ung (0 = chua thiet lap)
    //   lastCmd / lastArg : lenh va tham so vua gui de xu ly phan hoi
    // ---------------------------------------------------------------
    std::string dataChannelIP   = serverIP;
    int         dataChannelPort = 0;
    std::string lastCmd         = "";
    std::string lastArg         = "";

    std::string inputLine;
    while (true) {
        // Hien thi trang thai data channel trong prompt neu da thiet lap
        if (dataChannelPort > 0) {
            std::cout << "ftp [DataReady:" << dataChannelPort << "]> ";
        } else {
            std::cout << "ftp> ";
        }

        // Doc dong lenh tu nguoi dung
        if (!std::getline(std::cin, inputLine)) break;  // EOF (Ctrl+Z/Ctrl+D)
        if (inputLine.empty()) continue;

        // Kiem tra va phan tich cu phap lenh
        std::string cmd, arg;
        if (!ClientCLI::parseAndValidate(inputLine, cmd, arg)) {
            continue;  // Cu phap sai, yeu cau nhap lai
        }

        // -----------------------------------------------------------
        // Xu ly lenh QUIT: gui Server roi thoat vong lap
        // -----------------------------------------------------------
        if (cmd == "QUIT") {
            std::cout << "  Dang ngat ket noi...\n";
            ftpClient.sendCommand("QUIT");
            std::string quitResp = ftpClient.readResponse();
            if (!quitResp.empty()) {
                std::cout << "  Server: " << quitResp;
            }
            break;
        }

        // -----------------------------------------------------------
        // Xu ly lenh HELP noi bo: hien thi bang lenh ma khong gui Server
        // -----------------------------------------------------------
        if (cmd == "HELP" && arg.empty()) {
            ClientCLI::printHelp();
            continue;
        }

        // -----------------------------------------------------------
        // Ghep lenh day du: "CMD" hoac "CMD argument"
        // -----------------------------------------------------------
        std::string fullCmd = arg.empty() ? cmd : (cmd + " " + arg);

        // -----------------------------------------------------------
        // Gui lenh len Server qua TCP Control Channel
        // -----------------------------------------------------------
        if (!ftpClient.sendCommand(fullCmd)) {
            std::cerr << "  [Loi] Gui lenh that bai. Ket noi co the da bi gian doan.\n";
            break;
        }

        // Luu lenh va tham so vua gui de xu ly phan hoi tuong ung
        lastCmd = cmd;
        lastArg = arg;

        // -----------------------------------------------------------
        // Doc phan hoi tu Server
        // -----------------------------------------------------------
        std::string response = ftpClient.readResponse();
        if (response.empty()) {
            // Khong nhan duoc phan hoi - Server co the da ngat ket noi
            std::cerr << "  [Loi] Khong nhan duoc phan hoi tu Server.\n";
            std::cerr << "        Ket noi co the da bi dong.\n";
            break;
        }

        // In phan hoi thu tu Server
        std::cout << "  Server: " << response;

        // Trich xuat ma phan hoi 3 chu so
        int code = ftpClient.getReplyCode(response);

        // Goi may trang thai xu ly ma phan hoi
        handleReplyCode(code, response);

        // -----------------------------------------------------------
        // XU LY DAC BIET THEO MA PHAN HOI
        // May trang thai (State Machine) chinh dieu phoi luong hoat dong
        // -----------------------------------------------------------
        switch (code) {

        // Ma 150: Server dang mo ket noi du lieu, san sang truyen file
        // Day la noi tich hop voi module UDP RDT cua Thanh vien 3
        case 150: {
            if (dataChannelPort == 0) {
                std::cerr << "  [Loi] Chua co Data Channel!\n";
                std::cerr << "        Hay gui lenh PASV hoac PORT truoc RETR/STOR/LIST.\n";
                break;
            }

            if (lastCmd == "RETR") {
                // Tai file tu Server ve may (Download)
                // Uu tien lay kich thuoc file truoc (neu da gui SIZE truoc do)
                uint64_t fileSize = 0;

                // Kiem tra file co the ghi duoc o dau ra khong
                {
                    std::ofstream testOut(lastArg, std::ios::binary);
                    if (!testOut.is_open()) {
                        std::cerr << "  [Loi] Khong tao duoc file dau ra: '"
                                  << lastArg << "'\n";
                        std::cerr << "        Kiem tra quyen ghi va duong dan.\n";
                        break;
                    }
                    testOut.close();
                    // Xoa file rong vua tao de RDT tu ghi lai
                    std::remove(lastArg.c_str());
                }

                std::cout << "  [RDT] Bat dau nhan file: '" << lastArg << "'\n";
                std::cout << "        Tu: " << dataChannelIP << ":" << dataChannelPort << "\n";

                // Goi ham RDT nhan file (Thanh vien 3 implement)
                // Ham nay doc theo che do nhi phan (std::ios::binary) de
                // dam bao khong lam hong file anh, video, file nen, v.v.
                bool recvOK = rdt_receive_file(lastArg, dataChannelIP,
                                               dataChannelPort, fileSize);

                if (recvOK) {
                    std::cout << "  [RDT] Nhan file thanh cong: " << lastArg << "\n";

                    // Doc ma ket thuc truyen (Ma 226 - Transfer complete)
                    // Server gui kem checksum trong phan hoi nay:
                    // Vi du: "226 Transfer complete. Checksum: 123456"
                    std::string doneResp = ftpClient.readResponse();
                    if (!doneResp.empty()) {
                        std::cout << "  Server: " << doneResp;
                        handleReplyCode(ftpClient.getReplyCode(doneResp), doneResp);

                        // Kiem tra toan ven bang Checksum
                        // Server (transfer_handler.cpp) tinh checksum bang cong thuc:
                        //   hash = (hash + byte) % 1000000007
                        // Client dung cung cong thuc, so sanh hai gia tri
                        long long serverChecksum = parseChecksumFrom226(doneResp);
                        if (serverChecksum >= 0) {
                            long long localChecksum = computeChecksum(lastArg);
                            std::cout << "  [Checksum] Server: " << serverChecksum << "\n";
                            std::cout << "  [Checksum] Local : " << localChecksum  << "\n";
                            if (localChecksum == serverChecksum) {
                                std::cout << "  [Checksum] KHOP! File nhan nguyen ven.\n";
                            } else {
                                std::cerr << "  [Checksum] KHONG KHOP! File co the bi loi!\n";
                            }
                        }
                    }
                } else {
                    std::cerr << "  [RDT] Nhan file that bai.\n";
                }

                // Reset data channel sau khi su dung xong
                dataChannelPort = 0;
            }
            else if (lastCmd == "STOR") {
                // Upload file len Server
                // Kiem tra file co ton tai o may Client khong
                uint64_t fileSize = 0;
                {
                    std::ifstream checkFile(lastArg, std::ios::binary | std::ios::ate);
                    if (!checkFile.is_open()) {
                        std::cerr << "  [Loi] Khong mo duoc file: '" << lastArg << "'\n";
                        std::cerr << "        Kiem tra file ton tai va co quyen doc.\n";
                        dataChannelPort = 0;
                        break;
                    }
                    // Lay kich thuoc file de hien thi thong tin truoc khi gui
                    fileSize = (uint64_t)checkFile.tellg();
                    checkFile.close();
                }

                std::cout << "  [RDT] Bat dau upload file: '" << lastArg << "'\n";
                std::cout << "        Kich thuoc: " << fileSize << " bytes\n";
                std::cout << "        Den: " << dataChannelIP << ":" << dataChannelPort << "\n";

                // Goi ham RDT gui file (Thanh vien 3 implement)
                // Ham phai doc file theo che do nhi phan (std::ios::binary)
                bool sendOK = rdt_send_file(lastArg, dataChannelIP, dataChannelPort);

                if (sendOK) {
                    std::cout << "  [RDT] Upload hoan tat: " << lastArg << "\n";

                    // Doc ma ket thuc truyen tu Server (Ma 226)
                    std::string doneResp = ftpClient.readResponse();
                    if (!doneResp.empty()) {
                        std::cout << "  Server: " << doneResp;
                        handleReplyCode(ftpClient.getReplyCode(doneResp), doneResp);
                    }
                } else {
                    std::cerr << "  [RDT] Upload that bai.\n";
                }

                dataChannelPort = 0;
            }
            else if (lastCmd == "LIST" || lastCmd == "NLST") {
                // Nhan danh sach thu muc qua UDP Data Channel
                std::cout << "  [RDT] Dang nhan danh sach thu muc...\n";

                // rdt_receive_listing: nhan chuoi van ban chua ket qua LIST
                std::string listing = rdt_receive_listing(dataChannelIP, dataChannelPort);

                if (!listing.empty()) {
                    std::cout << "\n  --- Noi dung thu muc ---\n";
                    std::cout << listing;
                    std::cout << "  ------------------------\n\n";
                }

                // Doc ma 226 bao ket thuc truyen
                std::string doneResp = ftpClient.readResponse();
                if (!doneResp.empty()) {
                    std::cout << "  Server: " << doneResp;
                }

                dataChannelPort = 0;
            }
            break;
        }

        // Ma 226: Truyen hoan tat (Transfer Complete)
        // Neu khong bat duoc ma 150 truoc (Server truyen nhanh ngay),
        // reset data channel de chuan bi cho luong tiep theo
        case 226: {
            std::cout << "  [Done] Truyen du lieu hoan tat thanh cong.\n";
            dataChannelPort = 0;
            break;
        }

        // Ma 227: Server tra loi PASV thanh cong, chua IP:Port cua Data Channel
        case 227: {
            std::string psvIP;
            int         psvPort = 0;

            if (ftpClient.parsePassiveResponse(response, psvIP, psvPort)) {
                dataChannelIP   = psvIP;
                dataChannelPort = psvPort;
                std::cout << "  [PASV] Data Channel da thiet lap:\n";
                std::cout << "         IP  : " << psvIP  << "\n";
                std::cout << "         Port: " << psvPort << "\n";
                std::cout << "  [PASV] San sang! Hay gui RETR <file>, STOR <file>, hoac LIST.\n";
            }
            break;
        }

        // Ma 200: Lenh PORT duoc chap nhan - Active Mode san sang
        case 200: {
            if (lastCmd == "PORT") {
                std::cout << "  [PORT] Active Mode da duoc thiet lap.\n";
                std::cout << "         Server se ket noi nguoc lai khi nhan RETR/STOR/LIST.\n";
            }
            break;
        }

        // Ma 230: Dang nhap thanh cong
        case 230: {
            std::cout << "  [Auth] Chao mung! Phien FTP da bat dau.\n";
            ClientCLI::printStatus(serverIP, serverPort, true);
            break;
        }

        // Ma 331: Server yeu cau mat khau sau ten dang nhap
        case 331: {
            // handleReplyCode() da in thong bao, khong can them o day
            break;
        }

        // Mac dinh: Khong co xu ly dac biet them
        default:
            break;
        }

    }  // Het vong lap CLI

    // ---------------------------------------------------------------
    // BUOC 6: Ngat ket noi va hien thi thong bao ket thuc
    // ---------------------------------------------------------------
    ftpClient.disconnect();
    ClientCLI::printStatus(serverIP, serverPort, false);
    std::cout << "  Phien FTP ket thuc. Tam biet!\n\n";
    return 0;
}