#include "../include/core/crypto_hash.h"
#include <iostream>
#include <fstream>
#include <windows.h>
#include <wincrypt.h>
#include <sstream>
#include <iomanip>

// Bắt buộc liên kết thư viện mật mã của Windows khi biên dịch
#pragma comment(lib, "advapi32.lib")

// Đặt kích thước bộ đệm là 8KB (8192 bytes)
#define BUFSIZE 8192 

std::string calculate_file_hash(const std::string& filepath) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    std::string hashString = "";
    
    // 1. Mở file ở chế độ nhị phân (binary mode) để không làm hỏng dữ liệu gốc
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[HASH_ERROR] Khong the mo tep: " << filepath << "\n";
        return "";
    }

    // 2. Khởi tạo môi trường xử lý của Windows API
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        std::cerr << "[HASH_ERROR] Loi khoi tao Cryptographic Context.\n";
        return "";
    }

    // 3. Khởi tạo đối tượng băm theo thuật toán MD5
    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return "";
    }

    // 4. Vòng lặp đọc tệp tin theo từng khối (Chunking)
    char buffer[BUFSIZE];
    
    // file.read() sẽ đọc tối đa 8KB mỗi vòng. 
    // file.gcount() lấy số byte thực tế vừa đọc được ở vòng lặp đó.
    while (file.read(buffer, BUFSIZE) || file.gcount() > 0) {
        DWORD bytesRead = static_cast<DWORD>(file.gcount());
        
        // Ép kiểu con trỏ char* của mảng thành BYTE* (unsigned char*) để API Windows hiểu
        if (!CryptHashData(hHash, reinterpret_cast<BYTE*>(buffer), bytesRead, 0)) {
            std::cerr << "[HASH_ERROR] Loi trong qua trinh bam du lieu.\n";
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return "";
        }
    }

    file.close(); // Đọc xong thì đóng tệp lại

    // 5. Trích xuất kết quả băm thô (MD5 luôn ra 16 bytes)
    DWORD hashLen = 16;
    BYTE hashBytes[16];
    
    if (CryptGetHashParam(hHash, HP_HASHVAL, hashBytes, &hashLen, 0)) {
        // 6. Chuyển đổi 16 byte nhị phân thành chuỗi Hex 32 ký tự để dễ hiển thị
        std::stringstream ss;
        for (DWORD i = 0; i < hashLen; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hashBytes[i]);
        }
        hashString = ss.str();
    }

    // 7. Giải phóng bộ nhớ con trỏ bắt buộc để tránh rò rỉ RAM
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    return hashString;
}