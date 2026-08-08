#pragma once

#include <string>
#include <cstdint>

class ClientCLI {
public:
    // Kiem tra tinh hop le cua cau lenh nguoi dung nhap
    // inputLine : chuoi nguoi dung nhap (VD: "RETR my file.pdf")
    // cmdOut    : [out] ten lenh in hoa (VD: "RETR")
    // argOut    : [out] tham so sau lenh (VD: "my file.pdf") - giu nguyen khoang trang
    // Tra ve    : true neu hop le, false neu sai cu phap
    static bool parseAndValidate(const std::string& inputLine,
                                 std::string& cmdOut,
                                 std::string& argOut);

    // Hien thi bang huong dan day du tat ca lenh FTP duoc ho tro (Muc 2.2)
    static void printHelp();



    // Hien thi trang thai ket noi mang len console
    // ip        : dia chi IP Server
    // port      : cong dich vu
    // connected : true = dang ket noi, false = da ngat
    static void printStatus(const std::string& ip, int port, bool connected);
};
