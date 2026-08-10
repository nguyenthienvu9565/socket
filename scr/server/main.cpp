#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>
#include <string>
#include <filesystem>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "session.h"
#include "command_dispatcher.h"
#include "reply_codes.h"
#include "../include/core/rdt.h" 

namespace {

constexpr int CONTROL_PORT = 2121; 
constexpr int BACKLOG = 16;

ftp::ClientRegistry g_clientRegistry;

} // namespace

void handleClientSession(SOCKET clientSocket, std::string clientId) {
    ftp::Session session;
    session.socketFd = clientSocket;
    session.clientId = clientId;
    session.rootDir = "./ftp_root"; 
    session.cwd = "";

    std::error_code ec;
    std::filesystem::create_directories(session.rootDir, ec);

    std::string welcome = ftp::formatReply(ftp::SERVICE_READY, "Hybrid FTP server ready");
    send(clientSocket, welcome.c_str(), static_cast<int>(welcome.size()), 0);

    char buf[4096];
    std::string recvBuffer; 

    while (true) {
        int n = recv(clientSocket, buf, sizeof(buf), 0);
        if (n <= 0) break; 

        recvBuffer.append(buf, static_cast<size_t>(n));

        size_t pos = 0;
        while ((pos = recvBuffer.find("\r\n")) != std::string::npos) {
            std::string line = recvBuffer.substr(0, pos);
            recvBuffer.erase(0, pos + 2); 

            std::string reply = ftp::handleCommand(line, session);
            send(clientSocket, reply.c_str(), static_cast<int>(reply.size()), 0);

            if (reply.rfind(std::to_string(ftp::GOODBYE), 0) == 0) {
                goto end_session; 
            }
        }
    }

end_session:
    closesocket(clientSocket);
    g_clientRegistry.remove(clientId); 
    std::cout << "[server] client disconnected: " << clientId << std::endl;
}

int main() {
    std::cout << "\n";
    std::cout << "  +==============================================================+\n";
    std::cout << "  |      HYBRID FTP CLIENT  -  Giao tiep thong nhat qua RDT     |\n";
    std::cout << "  |      Phien ban: 5.0 | Kiem tra toan ven bang MD5 Hash       |\n";
    std::cout << "  +==============================================================+\n\n";

    if (!rdt_init()) {
        std::cerr << "rdt_init() failed\n";
        return 1;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "socket() failed\n";
        rdt_cleanup();
        return 1;
    }

    int opt = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(CONTROL_PORT);

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "bind() failed\n";
        closesocket(listenSocket);
        rdt_cleanup();
        return 1;
    }
    if (listen(listenSocket, BACKLOG) == SOCKET_ERROR) {
        std::cerr << "listen() failed\n";
        closesocket(listenSocket);
        rdt_cleanup();
        return 1;
    }

    std::cout << "[server] listening on port " << CONTROL_PORT << std::endl;

    while (true) {
        sockaddr_in clientAddr{};
        int clientAddrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket,
                                   reinterpret_cast<sockaddr*>(&clientAddr),
                                   &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) continue;

        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);

        std::string clientId = std::string(clientIp) + ":" + std::to_string(ntohs(clientAddr.sin_port));
        
        g_clientRegistry.add(clientId);
        std::cout << "[server] client connected: " << clientId << std::endl;

        std::thread(handleClientSession, clientSocket, clientId).detach();
    }

    closesocket(listenSocket);
    rdt_cleanup();
    return 0;
}