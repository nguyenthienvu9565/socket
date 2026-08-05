#pragma once

#include <string>

// Hàm nhận vào đường dẫn tệp tin vật lý và trả về chuỗi băm MD5 dài 32 ký tự.
// Trả về chuỗi rỗng ("") nếu có lỗi xảy ra (ví dụ: file không tồn tại).
std::string calculate_file_hash(const std::string& filepath);