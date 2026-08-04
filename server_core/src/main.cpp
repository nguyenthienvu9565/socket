#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <algorithm>

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

// TODO(you): if you need the "active session table" from spec
// section 4.5, promote this into the ClientRegistry class sketched
// in session.h so other code (e.g. a STAT/admin command) can query
// it too. For now it's just used for the connect/disconnect log.
std::mutex g_clientsMutex;
std::vector<std::string> g_connectedClients; // protected by g_clientsMutex

} // namespace

// Runs entirely on its own thread — one per connected client. This is
// the "Concurrency Control" the spec grades: each thread gets its own
// Session (isolated cwd/auth/rename state), and the only shared state
// it touches (g_connectedClients) is mutex-protected.
void handleClientSession(int clientSocket, std::string clientId) {
    ftp::Session session;
    session.socketFd = clientSocket;
    session.clientId = clientId;
    session.rootDir = "./ftp_root"; // TODO(you): per-user root vs shared root — your call
    session.cwd = "";

    std::string welcome = ftp::formatReply(ftp::SERVICE_READY, "Hybrid FTP server ready");
    send(clientSocket, welcome.c_str(), welcome.size(), 0);

    char buf[4096];
    while (true) {
        ssize_t n = recv(clientSocket, buf, sizeof(buf), 0);
        if (n <= 0) break; // client disconnected or socket error

        // TODO(you): this treats one recv() as one command line, which
        // is NOT guaranteed over TCP — a single recv() can contain
        // zero, one, or several "\r\n"-terminated lines, and a line
        // can be split across two recv() calls. Before relying on
        // this under real network conditions, buffer incoming bytes
        // and split on "\r\n" yourself, keeping any trailing partial
        // line for the next recv().
        std::string line(buf, static_cast<size_t>(n));
        std::string reply = ftp::handleCommand(line, session);

        send(clientSocket, reply.c_str(), reply.size(), 0);

        if (reply.rfind(std::to_string(ftp::GOODBYE), 0) == 0) {
            break; // QUIT was handled — close this session
        }
    }

    close(clientSocket);
    {
        std::lock_guard<std::mutex> lock(g_clientsMutex);
        auto& v = g_connectedClients;
        v.erase(std::remove(v.begin(), v.end(), clientId), v.end());
    }
    std::cout << "[server] client disconnected: " << clientId << std::endl;
}

int main() {
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        std::cerr << "socket() failed\n";
        return 1;
    }

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
        {
            std::lock_guard<std::mutex> lock(g_clientsMutex);
            g_connectedClients.push_back(clientId);
        }
        std::cout << "[server] client connected: " << clientId << std::endl;

        // detach(): this thread lives on independently of this accept
        // loop until the client disconnects.
        // TODO(you): consider keeping std::thread handles (e.g. in a
        // vector) and joining them on shutdown instead of detaching,
        // if you need the server to exit cleanly rather than being
        // killed.
        std::thread(handleClientSession, clientSocket, clientId).detach();
    }

    close(listenSocket);
    return 0;
}
