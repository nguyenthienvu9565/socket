#include "../../../include/server/handlers/auth_handler.h"
#include "../../../include/server/reply_codes.h"
#include <unordered_map>

namespace ftp {

// In-memory user credential table
static const std::unordered_map<std::string, std::string> USER_CREDENTIALS = {
    {"admin", "secret123"},
    {"guest", "guestpass"},
    {"anonymous", ""} // Hỗ trợ đăng nhập ẩn danh không cần pass
};

std::string handleUSER(const std::string& args, Session& session) {
    if (args.empty()) {
        return formatReply(SYNTAX_ERROR_PARAMS, "Username required");
    }
    session.username = args;
    session.authenticated = false;
    return formatReply(USERNAME_OK_NEED_PASS, "User name okay, need password");
}

std::string handlePASS(const std::string& args, Session& session) {
    if (session.username.empty()) {
        return formatReply(SYNTAX_ERROR, "Send USER first");
    }

    bool ok = false;
    auto it = USER_CREDENTIALS.find(session.username);
    if (it != USER_CREDENTIALS.end() && it->second == args) {
        ok = true;
    } else if (session.username == "anonymous") {
        ok = true;
    }

    if (!ok) {
        session.username.clear();
        return formatReply(NOT_LOGGED_IN, "Login incorrect");
    }
    session.authenticated = true;
    return formatReply(LOGIN_SUCCESSFUL, "Login successful");
}

std::string handleQUIT(Session& session) {
    session.authenticated = false;
    return formatReply(GOODBYE, "Goodbye");
}

} // namespace ftp