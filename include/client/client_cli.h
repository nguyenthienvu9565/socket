#pragma once

#include <string>

class ClientCLI {
public:
    // Kiểm tra tính hợp lệ của câu lệnh người dùng gõ vào
    static bool parseAndValidate(const std::string& inputLine, std::string& cmdOut, std::string& argOut);
    
    // Hiển thị menu hướng dẫn
    static void printHelp();
};
