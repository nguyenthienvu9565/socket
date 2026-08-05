#include "command_dispatcher.h"
#include "reply_codes.h"
#include "handlers/auth_handler.h"
#include "handlers/fs_handler.h"
#include <algorithm>
#include <utility>

namespace ftp {

namespace {

// Splits "STOR myfile.txt" into verb="STOR", args="myfile.txt".
// TODO(you): commands with multiple comma-separated args (like
// "PORT h1,h2,h3,h4,p1,p2") get their whole tail as one `args` string
// here — have that handler split further on ',' itself.
std::pair<std::string, std::string> splitCommand(const std::string& line) {
    std::string trimmed = line;
    while (!trimmed.empty() && (trimmed.back() == '\r' || trimmed.back() == '\n')) {
        trimmed.pop_back();
    }
    auto spacePos = trimmed.find(' ');
    if (spacePos == std::string::npos) {
        std::string verb = trimmed;
        std::transform(verb.begin(), verb.end(), verb.begin(), ::toupper);
        return {verb, ""};
    }
    std::string verb = trimmed.substr(0, spacePos);
    std::transform(verb.begin(), verb.end(), verb.begin(), ::toupper);
    std::string args = trimmed.substr(spacePos + 1);
    return {verb, args};
}

} // namespace

std::string handleCommand(const std::string& line, Session& session) {
    auto parsed = splitCommand(line);
    const std::string& verb = parsed.first;
    const std::string& args = parsed.second;

    // Commands allowed before authentication.
    if (verb == "USER") return handleUSER(args, session);
    if (verb == "PASS") return handlePASS(args, session);
    if (verb == "QUIT") return handleQUIT(session);
    if (verb == "NOOP") return formatReply(COMMAND_OK, "NOOP ok");

    // Everything else requires an authenticated session.
    if (!session.authenticated) {
        return formatReply(NOT_LOGGED_IN, "Not logged in");
    }

    // Filesystem / navigation commands — fully wired up.
    if (verb == "PWD")  return handlePWD(session);
    if (verb == "CWD")  return handleCWD(args, session);
    if (verb == "CDUP") return handleCDUP(session);
    if (verb == "MKD")  return handleMKD(args, session);
    if (verb == "RMD")  return handleRMD(args, session);
    if (verb == "LIST") return handleLIST(args, session);
    if (verb == "NLST") return handleNLST(args, session);
    if (verb == "SIZE") return handleSIZE(args, session);
    if (verb == "MDTM") return handleMDTM(args, session);

    // TODO(you): transfer-setup commands (need PORT/PASV state on
    // Session — fields already exist, logic doesn't yet):
    // if (verb == "TYPE") return handleTYPE(args, session);
    // if (verb == "MODE") return handleMODE(args, session);
    // if (verb == "PORT") return handlePORT(args, session);
    // if (verb == "PASV") return handlePASV(args, session);

    // TODO(you): transfer commands — these need an IRDTChannel, which
    // isn't available at this call site yet (handleCommand only gets
    // `session`). You'll likely need to either (a) open the RDT
    // channel here and pass it through, or (b) restructure so
    // main.cpp's client loop calls the transfer handlers directly
    // for RETR/STOR instead of routing them through this dispatcher.
    // Decide this with the group — it affects Member 1's interface
    // too. See handlers/transfer_handler.h for the function
    // signatures once you decide.
    // if (verb == "RETR") ...
    // if (verb == "STOR") ...
    // if (verb == "STOU") ...
    // if (verb == "APPE") ...
    // if (verb == "DELE") ...
    // if (verb == "RNFR") ...
    // if (verb == "RNTO") ...
    // if (verb == "HASH") ...
    // if (verb == "ABOR") ...
    // if (verb == "HELP") ...

    return formatReply(COMMAND_NOT_IMPLEMENTED, "Command not implemented: " + verb);
}

} // namespace ftp
