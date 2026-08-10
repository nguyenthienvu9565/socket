#pragma once
#include <string>
#include <filesystem>
#include <vector>
#include <memory>
#include <mutex>
#include <algorithm>
#include "rdt_interface.h"

namespace ftp {

struct Session {
    int socketFd = -1;
    std::string clientId;              

    bool authenticated = false;
    std::string username;

    std::filesystem::path rootDir;
    std::filesystem::path cwd;         
    std::string renameFromPath;

    bool passiveMode = false;
    std::string dataPeerHost;          
    int dataPeerPort = -1;

    // Owns the data channel for the CURRENT transfer setup, so it
    // survives between the PASV/PORT call and the later LIST/RETR/STOR
    // call that actually uses it. Each FTP command is a separate call
    // to handleCommand() — a channel created as a local variable
    // inside the PASV branch would be destroyed the moment that call
    // returns, leaving nothing for the next command to bind to.
    // nullptr until PASV or PORT sets one up; reset back to nullptr
    // once a transfer finishes (see command_dispatcher.cpp).
    std::unique_ptr<IRDTChannel> dataChannel;
};

// Hoàn thành TODO: Bảng Session Table (Concurrency Control)
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