cmake_minimum_required(VERSION 3.10)
project(hybrid_ftp_server_core CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server_core
    src/main.cpp
    src/command_dispatcher.cpp
    src/reply_codes.cpp
    src/mock_rdt.cpp
    src/handlers/auth_handler.cpp
    src/handlers/fs_handler.cpp
    src/handlers/transfer_handler.cpp
)

target_include_directories(server_core PRIVATE include)
find_package(Threads REQUIRED)
target_link_libraries(server_core PRIVATE Threads::Threads)
