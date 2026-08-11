#include "../../include/server/real_rdt.h"
#include "../../include/core/rdt.h"
#include <ws2tcpip.h>
#include <iostream>

namespace ftp {

RealRDTChannel::RealRDTChannel() {}

RealRDTChannel::~RealRDTChannel() {
    close();
}

void RealRDTChannel::applyRecvTimeout() {
    DWORD timeout = 3000;
    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
}

bool RealRDTChannel::open(const std::string& peerHost, int peerPort) {
    sock_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_ == INVALID_SOCKET) return false;
    applyRecvTimeout();

    peerAddr_.sin_family = AF_INET;
    peerAddr_.sin_port = htons(static_cast<uint16_t>(peerPort));
    if (inet_pton(AF_INET, peerHost.c_str(), &peerAddr_.sin_addr) <= 0) {
        return false;
    }
    peerConnected_ = true;
    return true;
}

int RealRDTChannel::bindPassive() {
    sock_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_ == INVALID_SOCKET) return -1;
    applyRecvTimeout();

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0;

    if (::bind(sock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        return -1;
    }

    sockaddr_in bound{};
    int len = sizeof(bound);
    if (getsockname(sock_, reinterpret_cast<sockaddr*>(&bound), &len) == SOCKET_ERROR) {
        return -1;
    }

    peerConnected_ = false;
    return ntohs(bound.sin_port);
}

bool RealRDTChannel::waitForPeer() {
    if (peerConnected_) return true;

    char probe[1];
    int peerLen = sizeof(peerAddr_);
    int peeked = ::recvfrom(sock_, probe, sizeof(probe), MSG_PEEK,
                                 reinterpret_cast<sockaddr*>(&peerAddr_), &peerLen);
    if (peeked == SOCKET_ERROR) return false;

    peerConnected_ = true;
    return true;
}

bool RealRDTChannel::sendChunk(const uint8_t* data, size_t len) {
    return false; // Deprecated, mapped to high-level APIs
}

long RealRDTChannel::receiveChunk(uint8_t* buffer, size_t bufferSize) {
    return 0; // Deprecated, mapped to high-level APIs
}

void RealRDTChannel::close() {
    if (sock_ != INVALID_SOCKET) {
        closesocket(sock_);
        sock_ = INVALID_SOCKET;
    }
    peerConnected_ = false;
}

bool RealRDTChannel::sendFile(const std::string& localPath) {
    if (!peerConnected_) return false;
    return rdt_send_file_stream(sock_, peerAddr_, localPath);
}

bool RealRDTChannel::receiveFile(const std::string& savePath) {
    if (!peerConnected_ && !waitForPeer()) return false;
    return rdt_receive_file_stream(sock_, savePath);
}

bool RealRDTChannel::sendBuffer(const std::vector<char>& data) {
    if (!peerConnected_) return false;
    return rdt_send_buffer(sock_, peerAddr_, data);
}

std::vector<char> RealRDTChannel::receiveBuffer() {
    if (!peerConnected_ && !waitForPeer()) return {};
    return rdt_receive_buffer(sock_);
}

} // namespace ftp
