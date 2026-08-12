cmake_minimum_required(VERSION 3.15)
project(HybridFTP_Project LANGUAGES CXX)

# 1. Thiết lập chuẩn C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 2. Định nghĩa macro cho Windows Vista+ (Để bật inet_pton và các API mạng)
if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0600 WIN32_LEAN_AND_MEAN)
endif()

# 3. Thêm thư mục chứa Header files
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 4. Gom các file mã nguồn (.cpp) theo từng nhóm
file(GLOB_RECURSE CORE_SOURCES   "src/core/*.cpp")
file(GLOB_RECURSE CLIENT_SOURCES "src/client/*.cpp")
file(GLOB_RECURSE SERVER_SOURCES "src/server/*.cpp")

# ==========================================================
# 5. BIÊN DỊCH CLIENT (client.exe)
# ==========================================================
add_executable(client ${CLIENT_SOURCES} ${CORE_SOURCES})
if(WIN32)
    target_link_libraries(client PRIVATE ws2_32)
endif()

# ==========================================================
# 6. BIÊN DỊCH SERVER (server.exe)
# ==========================================================
add_executable(server ${SERVER_SOURCES} ${CORE_SOURCES})
if(WIN32)
    target_link_libraries(server PRIVATE ws2_32)
endif()