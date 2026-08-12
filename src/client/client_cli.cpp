#include "../../include/client/client_cli.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <set>
#include <string>


static std::string trimWhitespace(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// ===================================================================
// parseAndValidate - Phan tich va kiem tra cu phap lenh FTP
//
// Cac nhom lenh duoc phan loai theo Muc 2.2 cua dac ta:
//   - NHOM 1: Lenh khong can tham so  (VD: PWD, PASV, QUIT)
//   - NHOM 2: Lenh can tham so bat buoc (VD: RETR, STOR, USER)
//   - NHOM 3: Lenh co tham so tuy chon  (VD: LIST, STAT, HELP)
//
// Diem quan trong: Dung std::getline de lay phan tham so thay vi >>
// vi >> se dung lai o khoang trang dau tien, lam mat ten file
// co chua dau cach (VD: "RETR tai lieu mon hoc.pdf")
// ===================================================================
bool ClientCLI::parseAndValidate(const std::string& inputLine,
                                  std::string& cmdOut,
                                  std::string& argOut) {
    std::stringstream ss(inputLine);
    ss >> cmdOut;
    if (cmdOut.empty()) return false;
    for ( int i = 0; i < cmdOut.length(); i++ )
    {
        cmdOut[i] = toupper ( cmdOut[i] );
    }
    std::string remaining;
    if (std::getline(ss, remaining)) {
        argOut = trimWhitespace(remaining);
    } else {
        argOut = "";
    }
    // NHOM 1: Lenh KHONG co tham so
    // Neu nguoi dung go them gi sau lenh, van chap nhan nhung bo qua
    static const std::set<std::string> noArgCmds = {
        "PWD",   // In thu muc hien tai
        "CDUP",  // Len thu muc cha
        "PASV",  // Bat Passive Mode
        "NOOP",  // Keep-alive ping
        "QUIT",  // Ket thuc phien
        "STOU",  // Upload voi ten doc nhat
        "ABOR"   // Huy truyen file
    };

    // NHOM 2: Lenh CO tham so BAT BUOC
    // Phai co it nhat 1 tham so, bao loi neu thieu
    static const std::set<std::string> requireArgCmds = {
        "USER",  // Ten dang nhap
        "PASS",  // Mat khau
        "CWD",   // Doi thu muc lam viec
        "MKD",   // Tao thu muc
        "RMD",   // Xoa thu muc
        "SIZE",  // Kich thuoc file
        "MDTM",  // Ngay chinh sua file
        "PORT",  // Thiet lap Active Mode (dinh dang h1,h2,h3,h4,p1,p2)
        "RETR",  // Tai file ve (Download)
        "STOR",  // Upload file len (Upload)
        "APPE",  // Noi tiep vao file co san
        "DELE",  // Xoa file
        "RNFR",  // Doi ten: chi dinh file nguon
        "RNTO",  // Doi ten: chi dinh ten moi
        "HASH"   // Kiem tra ma bam toan ven
    };

    // NHOM 3: Lenh co tham so TUY CHON
    // Co hoac khong deu duoc
    static const std::set<std::string> optArgCmds = {
        "LIST",  // Liet ke thu muc chi tiet
        "NLST",  // Liet ke ten file
        "STAT",  // Trang thai Server hoac metadata
        "TYPE",  // Che do truyen (A=ASCII, I=Binary)
        "MODE",  // Che do stream (S=Stream, B=Block, C=Compressed)
        "HELP"   // Tro giup
    };

    // ---------------------------------------------------------------
    // Kiem tra hop le theo nhom
    // ---------------------------------------------------------------
    if (noArgCmds.count(cmdOut) > 0) {
        return true;
    }
    else if (requireArgCmds.count(cmdOut) > 0) {
        if (argOut.empty()) {
            std::cout << "\n  [CLI Error] Lenh '" << cmdOut << "' can tham so.\n";
            std::cout << "             Vi du su dung: " << cmdOut << " <tham_so>\n\n";
            return false;
        }
        return true;
    }
    else if (optArgCmds.count(cmdOut) > 0) {
        return true;
    }
    else {
        std::cout << "\n  [CLI Error] Lenh '" << cmdOut << "' khong duoc nhan dang.\n";
        std::cout << "             Go 'HELP' de xem danh sach lenh duoc ho tro.\n\n";
        return false;
    }
}


void ClientCLI::printStatus(const std::string& ip, int port, bool connected) {
    if (connected) {
        std::cout << "\n  [Connected]    Server: " << ip << ":" << port << "\n\n";
    } else {
        std::cout << "\n  [Disconnected] Da ngat ket noi khoi Server.\n\n";
    }
}

void ClientCLI::printHelp() {
    std::cout << "\n";
    std::cout << "  +================================================================+\n";
    std::cout << "  |           HYBRID FTP CLIENT - DANH SACH LENH HO TRO           |\n";
    std::cout << "  +================================================================+\n";

    std::cout << "\n  --- XAC THUC (Authentication) ---\n";
    std::cout << "  USER <username>     : Gui ten dang nhap de bat dau phien\n";
    std::cout << "  PASS <password>     : Gui mat khau xac thuc\n";
    std::cout << "  QUIT                : Ket thuc phien va dong ket noi\n";
    std::cout << "  NOOP                : Keep-alive ping, giu session khoi timeout\n";

    std::cout << "\n  --- QUAN LY THU MUC (Directory Management) ---\n";
    std::cout << "  PWD                 : Hien thi duong dan thu muc hien tai tren Server\n";
    std::cout << "  CWD  <path>         : Chuyen thu muc lam viec sang <path>\n";
    std::cout << "  CDUP                : Len thu muc cha (tuong duong 'cd ..')\n";
    std::cout << "  MKD  <dirname>      : Tao thu muc moi tren Server\n";
    std::cout << "  RMD  <dirname>      : Xoa thu muc rong khoi Server\n";
    std::cout << "  LIST [path]         : Liet ke chi tiet (ten, kich thuoc, quyen...)\n";
    std::cout << "  NLST [path]         : Liet ke ten file thuan tuy\n";

    std::cout << "\n  --- THONG TIN FILE (File Information) ---\n";
    std::cout << "  STAT [path]         : Trang thai Server, hoac metadata file/thu muc\n";
    std::cout << "  SIZE <filename>     : Lay kich thuoc chinh xac (bytes) cua file\n";
    std::cout << "  MDTM <filename>     : Lay thoi diem chinh sua cuoi (YYYYMMDDhhmmss)\n";
    std::cout << "  HASH <filename>     : Lay ma bam (MD5/SHA-256) de kiem tra toan ven\n";

    std::cout << "\n  --- THIET LAP KENH DU LIEU (Data Channel Setup) ---\n";
    std::cout << "  TYPE {A|I}          : Che do truyen: A=ASCII, I=Image/Binary\n";
    std::cout << "  MODE {S|B|C}        : Che do stream: S=Stream, B=Block, C=Compressed\n";
    std::cout << "  PASV                : [Passive] Server mo port, Client ket noi vao\n";
    std::cout << "  PORT <h1,...,p2>    : [Active]  Client thong bao IP:Port cho Server ket noi\n";

    std::cout << "\n  --- TRUYEN FILE qua UDP Data Channel (RDT) ---\n";
    std::cout << "  RETR <filename>     : Tai file tu Server ve may (Download)\n";
    std::cout << "  STOR <filename>     : Upload file tu may len Server\n";
    std::cout << "  STOU                : Upload voi ten file doc nhat (Server tu dat ten)\n";
    std::cout << "  APPE <filename>     : Noi tiep du lieu vao file da co tren Server\n";

    std::cout << "\n  --- QUAN LY FILE (File Management) ---\n";
    std::cout << "  DELE <filename>     : Xoa file tren Server\n";
    std::cout << "  RNFR <oldname>      : Doi ten - Chi dinh file goc (bat buoc theo sau RNTO)\n";
    std::cout << "  RNTO <newname>      : Doi ten - Hoan thanh viec doi ten\n";

    std::cout << "\n  --- KHAC (Other) ---\n";
    std::cout << "  ABOR                : Huy luong truyen du lieu dang chay\n";
    std::cout << "  HELP [command]      : Hien thi tro giup (mac dinh: toan bo lenh)\n";

    std::cout << "\n  +================================================================+\n";
    std::cout << "  | Luu y: Truyen file RETR/STOR yeu cau thiet lap kenh du lieu    |\n";
    std::cout << "  |        bang PASV (Passive) HOAC PORT (Active) truoc.           |\n";
    std::cout << "  +================================================================+\n\n";
}