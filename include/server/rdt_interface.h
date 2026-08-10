#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace ftp {

class IRDTChannel {
public:
    virtual ~IRDTChannel() = default;

    virtual bool open(const std::string& peerHost, int peerPort) = 0;
    virtual int bindPassive() = 0;
    virtual bool waitForPeer() = 0;
    
    // API cũ (chỉ giữ lại chữ ký để tương thích interface, không còn dùng thực tế)
    virtual bool sendChunk(const uint8_t* data, size_t len) = 0;
    virtual long receiveChunk(uint8_t* buffer, size_t bufferSize) = 0;
    virtual void close() = 0;

    // Mở rộng API ánh xạ trực tiếp sang Core's bulk transfer
    virtual bool sendFile(const std::string& localPath) { return false; }
    virtual bool receiveFile(const std::string& savePath) { return false; }
    virtual bool sendBuffer(const std::vector<char>& data) { return false; }
    virtual std::vector<char> receiveBuffer() { return {}; }
};

} // namespace ftp