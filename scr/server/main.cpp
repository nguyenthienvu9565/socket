#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>
#include <string>
#include <filesystem>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "session.h"
#include "command_dispatcher.h"
#include "reply_codes.h"

namespace {

constexpr int CONTROL_PORT = 2121; // non-privileged port for local testing
constexpr int BACKLOG = 16;

// TODO Fulfilled: Sử dụng ClientRegistry từ session.h để quản lý
ftp::ClientRegistry g_clientRegistry;

} // namespace

void handleClientSession(int clientSocket, std::string clientId) {
    ftp::Session session;
    session.socketFd = clientSocket;
    session.clientId = clientId;
    session.rootDir = "./ftp_root"; 
    session.cwd = "";

    // Đảm bảo thư mục root tồn tại trước khi client thao tác
    std::error_code ec;
    std::filesystem::create_directories(session.rootDir, ec);

    std::string welcome = ftp::formatReply(ftp::SERVICE_READY, "Hybrid FTP server ready");
    send(clientSocket, welcome.c_str(), welcome.size(), 0);

    char buf[4096];
    std::string recvBuffer; // Bộ đệm chứa các byte chưa phân giải

    while (true) {
        ssize_t n = recv(clientSocket, buf, sizeof(buf), 0);
        if (n <= 0) break; // Client ngắt kết nối hoặc lỗi socket

        // TODO Fulfilled: Triển khai bộ đệm TCP an toàn
        // Cộng dồn dữ liệu mới nhận vào bộ đệm tổng
        recvBuffer.append(buf, static_cast<size_t>(n));

        size_t pos = 0;
        // Quét và trích xuất từng dòng lệnh hoàn chỉnh (kết thúc bằng \r\n)
        while ((pos = recvBuffer.find("\r\n")) != std::string::npos) {
            std::string line = recvBuffer.substr(0, pos);
            recvBuffer.erase(0, pos + 2); // Xóa dòng đã xử lý và chuỗi \r\n khỏi bộ đệm

            std::string reply = ftp::handleCommand(line, session);
            send(clientSocket, reply.c_str(), reply.size(), 0);

            if (reply.rfind(std::to_string(ftp::GOODBYE), 0) == 0) {
                goto end_session; // Thoát hoàn toàn nếu nhận lệnh QUIT
            }
        }
    }

end_session:
    close(clientSocket);
    g_clientRegistry.remove(clientId); // Xóa client khỏi danh sách quản lý chung
    std::cout << "[server] client disconnected: " << clientId << std::endl;
}

int main() {
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        std::cerr << "socket() failed\n";
        return 1;
    }

    // Cho phép tái sử dụng port ngay lập tức sau khi tắt server
    int opt = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(CONTROL_PORT);

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "bind() failed\n";
        return 1;
    }
    if (listen(listenSocket, BACKLOG) < 0) {
        std::cerr << "listen() failed\n";
        return 1;
    }

    std::cout << "[server] listening on port " << CONTROL_PORT << std::endl;

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSocket = accept(listenSocket,
                                   reinterpret_cast<sockaddr*>(&clientAddr),
                                   &clientAddrLen);
        if (clientSocket < 0) continue;

        std::string clientId = std::string(inet_ntoa(clientAddr.sin_addr)) +
                                ":" + std::to_string(ntohs(clientAddr.sin_port));
        
        // Thêm client vào bảng phiên (session table)
        g_clientRegistry.add(clientId);
        std::cout << "[server] client connected: " << clientId << std::endl;

        // Tách thread để xử lý client độc lập, cho phép vòng lặp chính tiếp tục accept client mới
        std::thread(handleClientSession, clientSocket, clientId).detach();
    }

    close(listenSocket);
    return 0;
}