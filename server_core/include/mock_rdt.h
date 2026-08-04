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
    bool sendChunk(const uint8_t* data, size_t len) override;
    long receiveChunk(uint8_t* buffer, size_t bufferSize) override;
    void close() override;

private:
    int sock_ = -1;
};

} // namespace ftp
