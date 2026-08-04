#include "handlers/auth_handler.h"
#include "reply_codes.h"

namespace ftp {

std::string handleUSER(const std::string& args, Session& session) {
    if (args.empty()) {
        return formatReply(SYNTAX_ERROR_PARAMS, "Username required");
    }
    session.username = args;
    session.authenticated = false; // must PASS before authenticated
    return formatReply(USERNAME_OK_NEED_PASS, "User name okay, need password");
}

std::string handlePASS(const std::string& args, Session& session) {
    if (session.username.empty()) {
        return formatReply(SYNTAX_ERROR, "Send USER first");
    }

    // TODO(you): replace this with a real credential check — a
    // user/password table (file, in-memory map, whatever your group
    // agreed on for "Basic Level: Authentication Mechanism").
    // Placeholder for now: accepts any non-empty password so you can
    // test the rest of the pipeline before auth logic is final.
    bool ok = !args.empty();

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
    // NOTE: this only updates session state / builds the reply text.
    // Closing the actual socket happens in main.cpp's client loop
    // after this reply is sent.
}

} // namespace ftp
