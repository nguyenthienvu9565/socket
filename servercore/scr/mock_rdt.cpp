#include "mock_rdt.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

namespace ftp {

void MockRDTChannel::applyRecvTimeout() {
    timeval tv{};
    tv.tv_sec = kIdleTimeoutSeconds;
    tv.tv_usec = 0;
    ::setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

bool MockRDTChannel::open(const std::string& peerHost, int peerPort) {
    sock_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_ < 0) return false;
    applyRecvTimeout();

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
    bool ok = ::connect(sock_, reinterpret_cast<sockaddr*>(&peerAddr), sizeof(peerAddr)) == 0;
    if (ok) peerConnected_ = true; // active mode: peer is known immediately
    return ok;
}

int MockRDTChannel::bindPassive() {
    sock_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_ < 0) return -1;
    applyRecvTimeout();

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0; // port 0 -> let the OS pick a free port

    if (::bind(sock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        return -1;
    }

    sockaddr_in bound{};
    socklen_t len = sizeof(bound);
    if (getsockname(sock_, reinterpret_cast<sockaddr*>(&bound), &len) < 0) {
        return -1;
    }

    // Peer is still unknown at this point — this is passive mode, so
    // we're waiting for the client to send to us first. receiveChunk()
    // learns and locks in the peer on that first packet.
    peerConnected_ = false;
    return ntohs(bound.sin_port);
}

bool MockRDTChannel::waitForPeer() {
    if (peerConnected_) return true; // active mode, or already learned

    // Peek at the client's first datagram just to learn its address —
    // MSG_PEEK leaves the datagram in the socket's queue so a later
    // receiveChunk() (e.g. STOR's read loop) still sees it as real
    // data, unconsumed by this call.
    uint8_t probe[1];
    sockaddr_in peerAddr{};
    socklen_t peerLen = sizeof(peerAddr);
    ssize_t peeked = ::recvfrom(sock_, probe, sizeof(probe), MSG_PEEK,
                                 reinterpret_cast<sockaddr*>(&peerAddr), &peerLen);
    if (peeked < 0) return false;

    if (::connect(sock_, reinterpret_cast<sockaddr*>(&peerAddr), peerLen) != 0) {
        return false;
    }
    peerConnected_ = true;
    return true;
}

bool MockRDTChannel::sendChunk(const uint8_t* data, size_t len) {
    if (!peerConnected_) {
        // Passive mode and the client hasn't sent us anything yet, so
        // there's no peer to send to. Callers doing RETR in passive
        // mode need the client to send first (e.g. a data-connection
        // open) before this can succeed — see the header note on
        // bindPassive().
        return false;
    }
    ssize_t sent = ::send(sock_, data, len, 0);
    return sent == static_cast<ssize_t>(len);
    // TODO(you): this is exactly the "unreliable" behavior the real
    // RDT layer has to fix — no ACK wait, no retry, no ordering.
}

long MockRDTChannel::receiveChunk(uint8_t* buffer, size_t bufferSize) {
    if (peerConnected_) {
        // A transfer is under way and we know who we're talking to.
        // Plain recv() is fine — and thanks to applyRecvTimeout(), if
        // nothing arrives for kIdleTimeoutSeconds we come back here
        // with EAGAIN/EWOULDBLOCK. Raw UDP has no "clean close" the
        // way TCP does, so treat that silence as the sender being
        // done: return 0, matching the interface's documented
        // clean-end-of-transfer contract. A genuine mid-transfer
        // network failure looks the same as this to the mock — the
        // real RDT layer should replace this with an explicit
        // end-of-transfer signal instead of relying on a timeout.
        ssize_t received = ::recv(sock_, buffer, bufferSize, 0);
        if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
            return -1;
        }
        return static_cast<long>(received);
    }

    // Passive mode, still waiting for the client's very first packet:
    // a timeout here means the client never connected at all, which
    // is a real failure, not "transfer complete" — nothing has
    // started yet.
    sockaddr_in peerAddr{};
    socklen_t peerLen = sizeof(peerAddr);
    ssize_t received = ::recvfrom(sock_, buffer, bufferSize, 0,
                                   reinterpret_cast<sockaddr*>(&peerAddr), &peerLen);
    if (received < 0) return -1;

    if (::connect(sock_, reinterpret_cast<sockaddr*>(&peerAddr), peerLen) == 0) {
        peerConnected_ = true;
    }
    return static_cast<long>(received);
}

void MockRDTChannel::close() {
    if (sock_ >= 0) {
        ::close(sock_);
        sock_ = -1;
    }
    peerConnected_ = false;
}

} // namespace ftp