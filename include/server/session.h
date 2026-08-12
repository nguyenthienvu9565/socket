#pragma once
#include <string>
#include <filesystem>
#include <vector>
#include <memory>
#include <mutex>
#include <algorithm>
#include <winsock2.h>
#include "rdt_interface.h"

namespace ftp {

struct Session {
    SOCKET socketFd = INVALID_SOCKET;
    std::string clientId;              

    bool authenticated = false;
    std::string username;

    std::filesystem::path rootDir;
    std::filesystem::path cwd;         
    std::string renameFromPath;

    bool passiveMode = false;
    std::string dataPeerHost;          
    int dataPeerPort = -1;

    std::unique_ptr<IRDTChannel> dataChannel;
    sockaddr_in clientControlAddr{};
};

class ClientRegistry {
public:
    void add(const std::string& clientId) {
        std::lock_guard<std::mutex> lock(mutex_);
        clients_.push_back(clientId);
    }

    void remove(const std::string& clientId) {
        std::lock_guard<std::mutex> lock(mutex_);
        clients_.erase(std::remove(clients_.begin(), clients_.end(), clientId), clients_.end());
    }

    std::vector<std::string> list() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return clients_;
    }
private:
    mutable std::mutex mutex_;
    std::vector<std::string> clients_; 
};

} // namespace ftp