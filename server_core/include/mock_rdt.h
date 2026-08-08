#pragma once
#include "rdt_interface.h"

namespace ftp {

// A throwaway stand-in for Member 1's real RDT channel, so you can
// compile and test your TCP / filesystem / binary-I/O code before
// their class exists. It does RAW, UNRELIABLE UDP — no ACKs, no
// retransmit, no ordering guarantees.
//
// Delete this once you swap in the real RDT class. Do NOT submit
// this as your reliability layer — an examiner who reads this file
// will immediately know it isn't one.
class MockRDTChannel : public IRDTChannel {
public:
    bool open(const std::string& peerHost, int peerPort) override;
    int bindPassive() override;
    bool waitForPeer() override;
    bool sendChunk(const uint8_t* data, size_t len) override;
    long receiveChunk(uint8_t* buffer, size_t bufferSize) override;
    void close() override;

private:
    int sock_ = -1;

    // True once the UDP peer is fixed — either open() connected out
    // to a known peer (active mode), or receiveChunk()/waitForPeer()
    // learned the peer from the client's first packet and connect()'d
    // to it (passive mode). sendChunk() needs this to be true.
    bool peerConnected_ = false;

    // How long receiveChunk() will wait for the NEXT datagram once a
    // transfer is already under way, before deciding the sender is
    // done. Raw UDP has no "clean close" signal the way TCP does, so
    // this idle timeout stands in for one: silence this long after at
    // least one real chunk has arrived means end-of-transfer, not an
    // error. Tune down for faster local testing, up if a real network
    // is involved. The real RDT layer should replace this entirely
    // with an explicit end-of-transfer signal from its protocol.
    static constexpr int kIdleTimeoutSeconds = 3;

    // Applies kIdleTimeoutSeconds as SO_RCVTIMEO on sock_. Call this
    // right after the socket is created (in open() and bindPassive()).
    void applyRecvTimeout();
};

} // namespace ftp