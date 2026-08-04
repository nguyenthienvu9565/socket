#include "mock_rdt.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

namespace ftp {

bool MockRDTChannel::open(const std::string& peerHost, int peerPort) {
    sock_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_ < 0) return false;

    sockaddr_in peerAddr{};
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_port = htons(static_cast<uint16_t>(peerPort));
    if (inet_pton(AF_INET, peerHost.c_str(), &peerAddr.sin_addr) <= 0) {
        return false;
    }
    // NOTE: connect() on a UDP socket just fixes the default peer for
    // send()/recv() below — it does not perform a handshake the way
    // TCP's connect() does. This is a mock; the real RDT class will
    // likely manage its own socket lifecycle differently.
    return ::connect(sock_, reinterpret_cast<sockaddr*>(&peerAddr), sizeof(peerAddr)) == 0;
}

bool MockRDTChannel::sendChunk(const uint8_t* data, size_t len) {
    ssize_t sent = ::send(sock_, data, len, 0);
    return sent == static_cast<ssize_t>(len);
    // TODO(you): this is exactly the "unreliable" behavior the real
    // RDT layer has to fix — no ACK wait, no retry, no ordering.
}

long MockRDTChannel::receiveChunk(uint8_t* buffer, size_t bufferSize) {
    ssize_t received = ::recv(sock_, buffer, bufferSize, 0);
    if (received < 0) return -1;
    return static_cast<long>(received);
}

void MockRDTChannel::close() {
    if (sock_ >= 0) {
        ::close(sock_);
        sock_ = -1;
    }
}

} // namespace ftp
