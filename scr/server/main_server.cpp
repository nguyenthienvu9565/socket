#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>
#include <string>
#include <filesystem>
#include <mutex>
#include <unordered_map>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "../../include/server/session.h"
#include "../../include/server/command_dispatcher.h"
#include "../../include/server/reply_codes.h"
#include "../../include/core/rdt.h" 

#pragma comment(lib, "ws2_32.lib")

namespace {

constexpr int CONTROL_PORT = 2121; 
ftp::ClientRegistry g_clientRegistry;

} // namespace

// Luong xu ly tung Client (su dung UDP Socket doc lap va RDT)
void handleClientSession(SOCKET clientSocket, std::string clientId, sockaddr_in clientAddr) {
    ftp::Session session;
    session.socketFd = clientSocket;
    session.clientId = clientId;
    session.rootDir = "./ftp_root"; 
    session.cwd = "";
    session.authenticated = false;

    std::error_code ec;
    std::filesystem::create_directories(session.rootDir, ec);

    std::string welcome = ftp::formatReply(ftp::SERVICE_READY, "Hybrid FTP server ready");
    std::vector<char> welcomePayload(welcome.begin(), welcome.end());
    rdt_send_buffer(clientSocket, clientAddr, welcomePayload);
    std::cout << "[server] sent 220 welcome to " << clientId << std::endl;

    while (true) {
        std::vector<char> reqData = rdt_receive_buffer(clientSocket);
        if (reqData.empty()) {
            break; 
        }

        std::string line(reqData.begin(), reqData.end());
        std::string logLine = line;
        while (!logLine.empty() && (logLine.back() == '\r' || logLine.back() == '\n')) {
            logLine.pop_back();
        }
        std::cout << "[server] " << clientId << " => " << logLine << std::endl;

        std::string reply = ftp::handleCommand(line, session);

        std::string logReply = reply;
        while (!logReply.empty() && (logReply.back() == '\r' || logReply.back() == '\n')) {
            logReply.pop_back();
        }
        std::cout << "[server] " << clientId << " <= " << logReply << std::endl;

        std::vector<char> replyPayload(reply.begin(), reply.end());
        rdt_send_buffer(clientSocket, clientAddr, replyPayload);

        if (reply.rfind(std::to_string(ftp::GOODBYE), 0) == 0) {
            break; 
        }
    }

    closesocket(clientSocket);
    g_clientRegistry.remove(clientId); 
    std::cout << "[server] client disconnected: " << clientId << std::endl;
}

int main() {
    std::cout << "\n";
    std::cout << "  +==============================================================+\n";
    std::cout << "  |      HYBRID FTP SERVER  -  Giao tiep thong nhat qua RDT     |\n";
    std::cout << "  |      Phien ban: 5.0 | Kiem tra toan ven bang MD5 Hash       |\n";
    std::cout << "  +==============================================================+\n\n";

    if (!rdt_init()) {
        std::cerr << "rdt_init() failed\n";
        return 1;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

    std::cout << "[server] listening on UDP port " << CONTROL_PORT << std::endl;

    while (true) {
        sockaddr_in clientAddr{};
        int clientAddrLen = sizeof(clientAddr);
        char buf[2048];
        
        int n = recvfrom(listenSocket, buf, sizeof(buf), 0,
                         reinterpret_cast<sockaddr*>(&clientAddr),
                         &clientAddrLen);
                         
        if (n <= 0) continue;

        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);
        std::string clientId = std::string(clientIp) + ":" + std::to_string(ntohs(clientAddr.sin_port));

        auto activeClients = g_clientRegistry.list();
        bool isKnown = (std::find(activeClients.begin(), activeClients.end(), clientId) != activeClients.end());

        if (!isKnown) {
            g_clientRegistry.add(clientId);
            std::cout << "[server] client connected: " << clientId << std::endl;

            SOCKET workerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            setsockopt(workerSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

            sockaddr_in localAddr{};
            localAddr.sin_family = AF_INET;
            localAddr.sin_addr.s_addr = INADDR_ANY;
            localAddr.sin_port = htons(CONTROL_PORT);
            bind(workerSocket, reinterpret_cast<sockaddr*>(&localAddr), sizeof(localAddr));

            connect(workerSocket, reinterpret_cast<sockaddr*>(&clientAddr), sizeof(clientAddr));

            std::thread(handleClientSession, workerSocket, clientId, clientAddr).detach();
        }
    }

    closesocket(listenSocket);
    rdt_cleanup();
    return 0;
}
