#include "../../../include/server/handlers/fs_handler.h"
#include "../../../include/server/reply_codes.h"
#include "../../../include/server/rdt_interface.h"
#include "../../../include/core/rdt.h"
#include <filesystem>
#include <optional>
#include <algorithm>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace fs = std::filesystem;

namespace ftp {

namespace {

std::optional<fs::path> resolveSafePath(const std::string& requested, const Session& session) {
    fs::path candidate = requested.empty()
        ? (session.rootDir / session.cwd)
        : (session.rootDir / session.cwd / requested);

    std::error_code ec;
    fs::path canonicalRoot = fs::weakly_canonical(session.rootDir, ec);
    if (ec) return std::nullopt;

    fs::path canonicalCandidate = fs::weakly_canonical(candidate, ec);
    if (ec) return std::nullopt;

    auto mismatch = std::mismatch(canonicalRoot.begin(), canonicalRoot.end(),
                                   canonicalCandidate.begin(), canonicalCandidate.end());
    if (mismatch.first != canonicalRoot.end()) {
        return std::nullopt; 
    }
    return canonicalCandidate;
}

std::string formatPermissions(fs::perms p) {
    std::string s = "rwxrwxrwx";
    if ((p & fs::perms::owner_read) == fs::perms::none) s[0] = '-';
    if ((p & fs::perms::owner_write) == fs::perms::none) s[1] = '-';
    if ((p & fs::perms::owner_exec) == fs::perms::none) s[2] = '-';
    if ((p & fs::perms::group_read) == fs::perms::none) s[3] = '-';
    if ((p & fs::perms::group_write) == fs::perms::none) s[4] = '-';
    if ((p & fs::perms::group_exec) == fs::perms::none) s[5] = '-';
    if ((p & fs::perms::others_read) == fs::perms::none) s[6] = '-';
    if ((p & fs::perms::others_write) == fs::perms::none) s[7] = '-';
    if ((p & fs::perms::others_exec) == fs::perms::none) s[8] = '-';
    return s;
}

} // namespace

sockaddr_in getPeerAddress(SOCKET sock) {
    sockaddr_in peerAddr{};
    int peerLen = sizeof(peerAddr);
    getpeername(sock, reinterpret_cast<sockaddr*>(&peerAddr), &peerLen);
    return peerAddr;
}

std::string handlePWD(Session& session) {
    return formatReply(PATHNAME_CREATED, "\"/" + session.cwd.generic_string() + "\" is the current directory");
}

std::string handleCWD(const std::string& args, Session& session) {
    auto resolved = resolveSafePath(args, session);
    if (!resolved || !fs::is_directory(*resolved)) {
        return formatReply(FILE_UNAVAILABLE, "Directory not found");
    }
    session.cwd = fs::relative(*resolved, session.rootDir);
    if (session.cwd == ".") session.cwd = ""; 
    return formatReply(FILE_ACTION_OK, "Directory changed to /" + session.cwd.generic_string());
}

std::string handleCDUP(Session& session) {
    return handleCWD("..", session);
}

std::string handleMKD(const std::string& args, Session& session) {
    if (args.empty()) return formatReply(SYNTAX_ERROR_PARAMS, "Directory name required");
    
    auto resolved = resolveSafePath(args, session);
    if (!resolved) {
        return formatReply(FILE_UNAVAILABLE, "Invalid directory path");
    }

    std::error_code ec;
    if (!fs::create_directory(*resolved, ec) || ec) {
        return formatReply(FILE_UNAVAILABLE, "Could not create directory");
    }
    return formatReply(PATHNAME_CREATED, "\"" + args + "\" directory created");
}

std::string handleRMD(const std::string& args, Session& session) {
    auto resolved = resolveSafePath(args, session);
    if (!resolved || !fs::is_directory(*resolved)) {
        return formatReply(FILE_UNAVAILABLE, "Directory not found");
    }
    std::error_code ec;
    if (!fs::remove(*resolved, ec) || ec) {
        return formatReply(FILE_UNAVAILABLE, "Could not remove directory");
    }
    return formatReply(FILE_ACTION_OK, "Directory removed");
}

std::string handleLIST(const std::string& args, Session& session, IRDTChannel& rdt) {
    auto resolved = resolveSafePath(args, session);
    if (!resolved || !fs::is_directory(*resolved)) {
        return formatReply(FILE_UNAVAILABLE, "Directory not found");
    }
    
    std::ostringstream out;
    for (const auto& entry : fs::directory_iterator(*resolved)) {
        std::error_code ec;
        auto perms = fs::status(entry, ec).permissions();
        
        out << (entry.is_directory() ? "d" : "-") << formatPermissions(perms) << " 1 ftp ftp ";
        if (entry.is_regular_file()) {
            out << std::setw(10) << fs::file_size(entry, ec) << " ";
        } else {
            out << std::setw(10) << 4096 << " "; 
        }
        out << entry.path().filename().string() << "\r\n";
    }

    std::string payload = out.str();

    if (payload.empty()) {
        payload = "(Directory is empty)\r\n";
    }

    std::vector<char> buffer(payload.begin(), payload.end());
    
    if (!rdt.sendBuffer(buffer)) {
         return formatReply(CONN_CLOSED_TRANSFER_ABORTED, "Data connection failed");
    }
    return formatReply(TRANSFER_COMPLETE, "Directory send OK.");
}

std::string handleNLST(const std::string& args, Session& session, IRDTChannel& rdt) {
    auto resolved = resolveSafePath(args, session);
    if (!resolved || !fs::is_directory(*resolved)) {
        return formatReply(FILE_UNAVAILABLE, "Directory not found");
    }
    
    std::ostringstream out;
    for (const auto& entry : fs::directory_iterator(*resolved)) {
        out << entry.path().filename().string() << "\r\n";
    }
    
    std::string payload = out.str();
    
    // Chống kẹt luồng RDT nếu thư mục rỗng (gói tin 0 byte)
    if (payload.empty()) {
        payload = "(Directory is empty)\r\n";
    }
    
    std::vector<char> buffer(payload.begin(), payload.end());
    
    // Gửi danh sách qua kênh dữ liệu Data Channel (UDP)
    if (!rdt.sendBuffer(buffer)) {
        return formatReply(CONN_CLOSED_TRANSFER_ABORTED, "Data connection failed");
    }
    
    // Trả về mã 226 báo hiệu truyền xong, thay vì nhồi dữ liệu vào mã 150 như cũ
    return formatReply(TRANSFER_COMPLETE, "Name list send OK.");
}

std::string handleSIZE(const std::string& args, Session& session) {
    auto resolved = resolveSafePath(args, session);
    if (!resolved || !fs::is_regular_file(*resolved)) {
        return formatReply(FILE_UNAVAILABLE, "File not found");
    }
    std::error_code ec;
    auto size = fs::file_size(*resolved, ec);
    if (ec) return formatReply(FILE_UNAVAILABLE, "Could not stat file");
    return formatReply(FILE_ACTION_OK, std::to_string(size));
}

std::string handleMDTM(const std::string& args, Session& session) {
    auto resolved = resolveSafePath(args, session);
    if (!resolved || !fs::is_regular_file(*resolved)) {
        return formatReply(FILE_UNAVAILABLE, "File not found");
    }

    auto ftime = fs::last_write_time(*resolved);
    auto sys_time = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
    );
    std::time_t cftime = std::chrono::system_clock::to_time_t(sys_time);
    std::tm* t = std::gmtime(&cftime);

    std::ostringstream ss;
    ss << std::put_time(t, "%Y%m%d%H%M%S");
    return formatReply(FILE_ACTION_OK, ss.str());
}
} // namespace ftp