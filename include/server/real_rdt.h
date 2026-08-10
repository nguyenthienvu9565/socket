#pragma once
#include "rdt_interface.h"
#include <winsock2.h>
#include <string>
#include <vector>

namespace ftp {

class RealRDTChannel : public IRDTChannel {
public:
    RealRDTChannel();
    ~RealRDTChannel() override;

    bool open(const std::string& peerHost, int peerPort) override;
    int bindPassive() override;
    bool waitForPeer() override;
    bool sendChunk(const uint8_t* data, size_t len) override;
    long receiveChunk(uint8_t* buffer, size_t bufferSize) override;
    void close() override;

    // High-level API mapping to Core
    bool sendFile(const std::string& localPath) override;
    bool receiveFile(const std::string& savePath) override;
    bool sendBuffer(const std::vector<char>& data) override;
    std::vector<char> receiveBuffer() override;

private:
    SOCKET sock_ = INVALID_SOCKET;
    sockaddr_in peerAddr_{};
    bool peerConnected_ = false;

    void applyRecvTimeout();
};

} // namespace ftp
