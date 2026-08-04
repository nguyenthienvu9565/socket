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

    // Opens/binds the UDP data channel for this transfer, to whichever
    // peer was negotiated via PORT/PASV.
    virtual bool open(const std::string& peerHost, int peerPort) = 0;

    // Sends exactly one chunk. Must block until Member 1's layer has
    // confirmed delivery (ACKed), or return false on failure.
    virtual bool sendChunk(const uint8_t* data, size_t len) = 0;

    // Receives exactly one chunk (used by STOR/uploads). Returns the
    // number of bytes written into buffer, 0 on clean end-of-transfer,
    // or -1 on error.
    virtual long receiveChunk(uint8_t* buffer, size_t bufferSize) = 0;

    virtual void close() = 0;
};

} // namespace ftp
