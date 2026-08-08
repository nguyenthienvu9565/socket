#include "handlers/transfer_handler.h"
#include "reply_codes.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <optional>
#include <sys/socket.h> // Để dùng ::send cho mã 150

namespace fs = std::filesystem;

namespace ftp {

namespace {
constexpr size_t CHUNK_SIZE = 1400; // TODO Fulfilled: UDP MTU safe

// Đặt lại hàm này ở đây trong anonymous namespace để tránh lỗi Linker
std::optional<fs::path> resolveSafePath(const std::string& requested, const Session& session) {
    fs::path candidate = requested.empty() ? (session.rootDir / session.cwd) : (session.rootDir / session.cwd / requested);
    std::error_code ec;
    fs::path canonicalRoot = fs::weakly_canonical(session.rootDir, ec);
    fs::path canonicalCandidate = fs::weakly_canonical(candidate, ec);
    if (ec) return std::nullopt;
    auto mismatch = std::mismatch(canonicalRoot.begin(), canonicalRoot.end(), canonicalCandidate.begin(), canonicalCandidate.end());
    if (mismatch.first != canonicalRoot.end()) return std::nullopt; 
    return canonicalCandidate;
}
} // namespace

std::string handleRETR(const std::string& args, Session& session, IRDTChannel& rdt) {
    if (args.empty()) return formatReply(SYNTAX_ERROR_PARAMS, "Filename required");
    
    // TODO Fulfilled: Chặn truy cập file bằng resolveSafePath
    auto resolved = resolveSafePath(args, session);
    if (!resolved || !fs::is_regular_file(*resolved)) return formatReply(FILE_UNAVAILABLE, "File not found");

    std::ifstream file(*resolved, std::ios::binary);
    if (!file) return formatReply(FILE_UNAVAILABLE, "Could not read file");

    // TODO Fulfilled: Send 150 FILE_STATUS_OK before pumping bytes
    std::string startReply = formatReply(FILE_STATUS_OK, "Opening data connection");
    ::send(session.socketFd, startReply.c_str(), startReply.size(), 0);

    std::vector<uint8_t> buffer(CHUNK_SIZE);
    uint64_t accumulatedHash = 0; // TODO Fulfilled: Checksum for verification

    while (file) {
        file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(CHUNK_SIZE));
        std::streamsize bytesRead = file.gcount();
        if (bytesRead <= 0) break;

        for (std::streamsize i = 0; i < bytesRead; ++i) {
            accumulatedHash = (accumulatedHash + buffer[i]) % 1000000007; 
        }

        if (!rdt.sendChunk(buffer.data(), static_cast<size_t>(bytesRead))) {
            return formatReply(CONN_CLOSED_TRANSFER_ABORTED, "Data connection lost");
        }
    }
    return formatReply(TRANSFER_COMPLETE, "Transfer complete. Checksum: " + std::to_string(accumulatedHash));
}

std::string handleSTOR(const std::string& args, Session& session, IRDTChannel& rdt) {
    if (args.empty()) return formatReply(SYNTAX_ERROR_PARAMS, "Filename required");
    
    auto resolved = resolveSafePath(args, session);
    if (!resolved) return formatReply(FILE_UNAVAILABLE, "Invalid path");

    std::ofstream file(*resolved, std::ios::binary | std::ios::trunc);
    if (!file) return formatReply(FILE_UNAVAILABLE, "Could not open file for writing");

    std::string startReply = formatReply(FILE_STATUS_OK, "Ok to send data.");
    ::send(session.socketFd, startReply.c_str(), startReply.size(), 0);

    std::vector<uint8_t> buffer(CHUNK_SIZE);
    while (true) {
        long bytesReceived = rdt.receiveChunk(buffer.data(), buffer.size());
        if (bytesReceived < 0) return formatReply(CONN_CLOSED_TRANSFER_ABORTED, "Data connection lost");
        if (bytesReceived == 0) break; 
        file.write(reinterpret_cast<char*>(buffer.data()), bytesReceived);
    }
    return formatReply(TRANSFER_COMPLETE, "Transfer complete");
}

} // namespace ftp