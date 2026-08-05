#include "handlers/transfer_handler.h"
#include "reply_codes.h"
#include <filesystem>
#include <fstream>
#include <vector>

namespace ftp {

namespace {
// TODO(you): tune this against whatever Member 1's RDT layer expects
// as a payload size. Their packet header + this payload must fit
// under the UDP-safe MTU (typically ~1400-1472 usable bytes before
// IP fragmentation kicks in on most networks) — confirm the exact
// number with them, it affects both your loop and their header math.
constexpr size_t CHUNK_SIZE = 1024;
} // namespace

std::string handleRETR(const std::string& args, Session& session, IRDTChannel& rdt) {
    if (args.empty()) {
        return formatReply(SYNTAX_ERROR_PARAMS, "Filename required");
    }
    std::filesystem::path filePath = session.rootDir / session.cwd / args;
    // TODO(you): route this through the same resolveSafePath-style
    // check fs_handler.cpp uses, or a shared helper — right now RETR
    // can be pointed outside rootDir the same way MKD currently can.

    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return formatReply(FILE_UNAVAILABLE, "File not found");
    }

    // TODO(you): send FILE_STATUS_OK (150) on the CONTROL channel
    // here (or from main.cpp's client loop, depending on how you
    // structure it) as the client's cue that data is now coming on
    // the data channel — before this function starts pumping bytes.

    std::vector<uint8_t> buffer(CHUNK_SIZE);
    while (file) {
        file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(CHUNK_SIZE));
        std::streamsize bytesRead = file.gcount();
        if (bytesRead <= 0) break;

        if (!rdt.sendChunk(buffer.data(), static_cast<size_t>(bytesRead))) {
            return formatReply(CONN_CLOSED_TRANSFER_ABORTED, "Data connection lost mid-transfer");
        }
    }

    // TODO(you): if you're doing end-to-end hash verification
    // (Excellent tier / HASH command), this loop is where you'd
    // accumulate the sender-side hash — you've just streamed the
    // whole file through it byte-for-byte.

    return formatReply(TRANSFER_COMPLETE, "Transfer complete");
}

std::string handleSTOR(const std::string& args, Session& session, IRDTChannel& rdt) {
    if (args.empty()) {
        return formatReply(SYNTAX_ERROR_PARAMS, "Filename required");
    }
    std::filesystem::path filePath = session.rootDir / session.cwd / args;

    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    if (!file) {
        return formatReply(FILE_UNAVAILABLE, "Could not open file for writing");
    }

    std::vector<uint8_t> buffer(CHUNK_SIZE);
    while (true) {
        long bytesReceived = rdt.receiveChunk(buffer.data(), buffer.size());
        if (bytesReceived < 0) {
            return formatReply(CONN_CLOSED_TRANSFER_ABORTED, "Data connection lost mid-transfer");
        }
        if (bytesReceived == 0) break; // clean end-of-transfer signal from RDT layer
        file.write(reinterpret_cast<char*>(buffer.data()), bytesReceived);
    }

    return formatReply(TRANSFER_COMPLETE, "Transfer complete");
}

} // namespace ftp
