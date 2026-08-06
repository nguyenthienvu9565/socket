#include <iostream>
#include <string>
#include "../../include/client/client_ftp.h"
#include "../../include/client/client_cli.h"

int main() {
    ClientFTP ftpClient;
    
    std::cout << "Dang ket noi toi Server...\n";
    if (!ftpClient.connectServer("127.0.0.1", 2121)) {
        std::cout << "Khong thể ket noi toi Server FTP!\n";
        return 1;
    }

    // Đọc thông điệp chào mừng (Mã 220)
    std::string welcomeMsg = ftpClient.readResponse();
    std::cout << welcomeMsg;

    ClientCLI::printHelp();

    std::string inputLine;
    while (true) {
        std::cout << "ftp> ";
        if (!std::getline(std::cin, inputLine) || inputLine.empty()) continue;

        std::string cmd, arg;
        if (!ClientCLI::parseAndValidate(inputLine, cmd, arg)) {
            continue; // Nhập sai cú pháp thì gõ lại
        }

        if (cmd == "QUIT") {
            ftpClient.sendCommand("QUIT");
            std::cout << ftpClient.readResponse();
            break;
        }

        // Gửi lệnh đã định dạng sang Server qua TCP Control Channel
        std::string fullCmd = arg.empty() ? cmd : (cmd + " " + arg);
        ftpClient.sendCommand(fullCmd);

        // Nhận phản hồi từ Server
        std::string response = ftpClient.readResponse();
        std::cout << response;

        int code = ftpClient.getReplyCode(response);

        // Nơi tích hợp UDP của Thành viên 3:
        // Nếu code == 150 (Opening UDP Data Connection), tiến hành gọi RDT Socket để nhận/gửi file!
    }

    ftpClient.disconnect();
    return 0;
}