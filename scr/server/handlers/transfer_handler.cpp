#include "../../../include/server/handlers/transfer_handler.h"
#include "../../../include/server/reply_codes.h"
#include "../../../include/core/crypto_hash.h"
#include "../../../include/core/rdt.h" 
#include <filesystem>
#include <fstream>
#include <vector>
#include <optional>
#include <winsock2.h> 

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

// Hàm hỗ trợ: Lấy địa chỉ của Client từ UDP socket đã connect
sockaddr_in getPeerAddress(SOCKET sock) {
    sockaddr_in peerAddr{};
    int peerLen = sizeof(peerAddr);
    getpeername(sock, reinterpret_cast<sockaddr*>(&peerAddr), &peerLen);
    return peerAddr;
}
} // namespace

std::string handleRETR(const std::string& args, Session& session, IRDTChannel& rdt) {
    if (args.empty()) return formatReply(SYNTAX_ERROR_PARAMS, "Filename required");
    
    auto resolved = resolveSafePath(args, session);
    if (!resolved || !fs::is_regular_file(*resolved)) return formatReply(FILE_UNAVAILABLE, "File not found");

    // Gửi mã 150 qua RDT
    std::string startReply = formatReply(FILE_STATUS_OK, "Opening data connection");
    std::vector<char> startPayload(startReply.begin(), startReply.end());
    rdt_send_buffer(session.socketFd, getPeerAddress(session.socketFd), startPayload);

    // Gửi file qua Data Channel
    if (!rdt.sendFile(resolved->string())) {
        return formatReply(CONN_CLOSED_TRANSFER_ABORTED, "Data connection lost");
    }

    // Tính MD5 và gửi nguyên chuỗi hash qua Data Channel
    std::string hash = calculate_file_hash(resolved->string());
    std::string hashMsg = hash + "\r\n";
    std::vector<char> hashPayload(hashMsg.begin(), hashMsg.end());
    rdt.sendBuffer(hashPayload);

    // Trả về mã 226
    return formatReply(TRANSFER_COMPLETE, "Transfer complete. MD5 sent over Data Channel.");
}

std::string handleSTOR(const std::string& args, Session& session, IRDTChannel& rdt) {
    if (args.empty()) return formatReply(SYNTAX_ERROR_PARAMS, "Filename required");
    
    auto resolved = resolveSafePath(args, session);
    if (!resolved) return formatReply(FILE_UNAVAILABLE, "Invalid path");

    // Gửi mã 150 qua RDT
    std::string startReply = formatReply(FILE_STATUS_OK, "Ok to send data.");
    std::vector<char> startPayload(startReply.begin(), startReply.end());
    rdt_send_buffer(session.socketFd, getPeerAddress(session.socketFd), startPayload);

    // Nhận file qua Data Channel
    if (!rdt.receiveFile(resolved->string())) {
        return formatReply(CONN_CLOSED_TRANSFER_ABORTED, "Data connection lost");
    }

    // Nhận hash từ Client qua Data Channel
    std::vector<char> hashData = rdt.receiveBuffer();
    std::string clientHash(hashData.begin(), hashData.end());
    
    // Loại bỏ khoảng trắng và xuống dòng
    while (!clientHash.empty() && (clientHash.back() == '\r' || clientHash.back() == '\n')) {
        clientHash.pop_back();
    }

    // Tính hash cục bộ
    std::string localHash = calculate_file_hash(resolved->string());
    
    if (clientHash == localHash) {
        return formatReply(TRANSFER_COMPLETE, "Transfer complete. Hash MATCHED: " + localHash);
    } else {
        return formatReply(TRANSFER_COMPLETE, "Transfer complete. Hash MISMATCH! Expected: " + clientHash);
    }
}

} // namespace ftp
