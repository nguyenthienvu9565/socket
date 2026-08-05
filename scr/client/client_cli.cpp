#include "../../include/client/client_cli.h"
#include <iostream>
#include <sstream>

bool ClientCLI::parseAndValidate(const std::string& inputLine, std::string& cmdOut, std::string& argOut) {
    std::stringstream ss(inputLine);
    ss >> cmdOut;
    
    if (cmdOut.empty()) return false;

    // Chuyển lệnh về dạng in hoa (USER, PASS, RETR, STOR, QUIT...)
    for (auto &c : cmdOut) c = toupper(c);

    // Kiểm tra các lệnh cần tham số
    if (cmdOut == "RETR" || cmdOut == "STOR" || cmdOut == "USER" || cmdOut == "PASS") {
        if (!(ss >> argOut)) {
            std::cout << "[CLI Error] Lenh " << cmdOut << " yeu cau tham so (vi du: " << cmdOut << " <filename>)\n";
            return false;
        }
    } else {
        // Lệnh không cần tham số hoặc lấy phần còn lại
        ss >> argOut;
    }

    return true;
}

void ClientCLI::printHelp() {
    std::cout << "\n================ HYBRID FTP CLIENT ================\n";
    std::cout << "  USER <username> : Dang nhap\n";
    std::cout << "  PASS <password> : Nhap mat khau\n";
    std::cout << "  RETR <filename> : Tai file tu server ve (UDP RDT)\n";
    std::cout << "  STOR <filename> : Upload file len server (UDP RDT)\n";
    std::cout << "  QUIT            : Thoat chuong trinh\n";
    std::cout << "===================================================\n\n";
}