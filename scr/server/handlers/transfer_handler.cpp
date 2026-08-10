#include "transfer_handler.h"
#include "reply_codes.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <optional>
#include <winsock2.h> 
#include "../../include/core/crypto_hash.h"

namespace fs = std::filesystem;

namespace ftp {

namespace {
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
    
    auto resolved = resolveSafePath(args, session);
    if (!resolved || !fs::is_regular_file(*resolved)) return formatReply(FILE_UNAVAILABLE, "File not found");

    std::string startReply = formatReply(FILE_STATUS_OK, "Opening data connection");
    ::send(session.socketFd, startReply.c_str(), static_cast<int>(startReply.size()), 0);

    if (!rdt.sendFile(resolved->string())) {
        return formatReply(CONN_CLOSED_TRANSFER_ABORTED, "Data connection lost");
    }

    std::string hash = calculate_file_hash(resolved->string());
    return formatReply(TRANSFER_COMPLETE, "Transfer complete. MD5: " + hash);
}

std::string handleSTOR(const std::string& args, Session& session, IRDTChannel& rdt) {
    if (args.empty()) return formatReply(SYNTAX_ERROR_PARAMS, "Filename required");
    
    auto resolved = resolveSafePath(args, session);
    if (!resolved) return formatReply(FILE_UNAVAILABLE, "Invalid path");

    std::string startReply = formatReply(FILE_STATUS_OK, "Ok to send data.");
    ::send(session.socketFd, startReply.c_str(), static_cast<int>(startReply.size()), 0);

    if (!rdt.receiveFile(resolved->string())) {
        return formatReply(CONN_CLOSED_TRANSFER_ABORTED, "Data connection lost");
    }

    std::string hash = calculate_file_hash(resolved->string());
    return formatReply(TRANSFER_COMPLETE, "Transfer complete. MD5: " + hash);
}

} // namespace ftp