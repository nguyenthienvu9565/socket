#pragma once
#include <cstdint>
#include <cstddef>
#include <string>

namespace ftp {

// This is the contract between your TCP / server-core code and
// Member 1's reliable-UDP (RDT) implementation. You code against
// this interface; Member 1 implements it as a concrete class.
//
// Agree on the EXACT signatures here with Member 1 before either of
// you writes the real logic — this boundary is where two halves of a
// group project usually stop compiling together.
//
// Until their real class exists, use MockRDTChannel (mock_rdt.h) so
// your own code compiles and is testable standalone.
class IRDTChannel {
public:
    virtual ~IRDTChannel() = default;

    // Opens the UDP data channel for ACTIVE mode: connects out to a
    // peer that's already known (from PORT).
    virtual bool open(const std::string& peerHost, int peerPort) = 0;

    // Opens the UDP data channel for PASSIVE mode: binds to an
    // OS-assigned local port instead of connecting to a known peer.
    // Returns the bound port on success, -1 on failure. The peer
    // address is NOT known at bind time — implementations must learn
    // it from the client's first incoming packet (see
    // MockRDTChannel::receiveChunk) before sendChunk() can be used.
    virtual int bindPassive() = 0;

    // Blocks until the client's data connection shows up on a
    // passively-bound channel, and locks in their address as the
    // peer — without consuming any real transfer data. Needed
    // because receiveChunk() is the only other way this mock learns
    // the peer, but send-first transfers (LIST, RETR) never call
    // receiveChunk() at all, so without this, sendChunk() would have
    // no peer to send to. No-op / returns true immediately if the
    // peer is already known (e.g. active mode via open()).
    virtual bool waitForPeer() = 0;

    // Sends exactly one chunk. Must block until Member 1's layer has
    // confirmed delivery (ACKed), or return false on failure. In
    // passive mode, must not be called before the peer has been
    // learned via waitForPeer() or receiveChunk() (see bindPassive()
    // note above).
    virtual bool sendChunk(const uint8_t* data, size_t len) = 0;

    // Receives exactly one chunk (used by STOR/uploads, and by
    // passive mode generally to learn the peer's address). Returns
    // the number of bytes written into buffer, 0 on clean
    // end-of-transfer, or -1 on error.
    virtual long receiveChunk(uint8_t* buffer, size_t bufferSize) = 0;

    virtual void close() = 0;
};

} // namespace ftp