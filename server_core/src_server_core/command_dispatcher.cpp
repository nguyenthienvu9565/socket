#include "command_dispatcher.h"
#include "reply_codes.h"
#include "handlers/auth_handler.h"
#include "handlers/fs_handler.h"
#include "handlers/transfer_handler.h" // Thêm header này
#include "mock_rdt.h"                  // Để khởi tạo MockRDTChannel
#include <algorithm>
#include <utility>
#include <vector>
#include <sstream>
#include <memory>

namespace ftp {

namespace {

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

// TODO Fulfilled: Tách arguments cho PORT bằng dấu phẩy
std::vector<std::string> splitCommaArgs(const std::string& args) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream stream(args);
    while (std::getline(stream, token, ',')) {
        tokens.push_back(token);
    }
    return tokens;
}

} // namespace

// GIỮ NGUYÊN CHỮ KÝ: Tương thích 100% với main.cpp
std::string handleCommand(const std::string& line, Session& session) {
    auto parsed = splitCommand(line);
    const std::string& verb = parsed.first;
    const std::string& args = parsed.second;

    if (verb == "USER") return handleUSER(args, session);
    if (verb == "PASS") return handlePASS(args, session);
    if (verb == "QUIT") return handleQUIT(session);
    if (verb == "NOOP") return formatReply(COMMAND_OK, "NOOP ok");

    if (!session.authenticated) return formatReply(NOT_LOGGED_IN, "Not logged in");

    if (verb == "PWD")  return handlePWD(session);
    if (verb == "CWD")  return handleCWD(args, session);
    if (verb == "CDUP") return handleCDUP(session);
    if (verb == "MKD")  return handleMKD(args, session);
    if (verb == "RMD")  return handleRMD(args, session);
    if (verb == "NLST") return handleNLST(args, session);
    if (verb == "SIZE") return handleSIZE(args, session);
    if (verb == "MDTM") return handleMDTM(args, session);

    // TODO Fulfilled: Transfer Setup (PORT/PASV)
    if (verb == "PORT") {
        auto parts = splitCommaArgs(args);
        if (parts.size() != 6) return formatReply(SYNTAX_ERROR_PARAMS, "Invalid PORT signature");
        try {
            session.passiveMode = false;
            session.dataChannel.reset(); // any earlier PASV channel is now stale
            session.dataPeerHost = parts[0] + "." + parts[1] + "." + parts[2] + "." + parts[3];
            session.dataPeerPort = (std::stoi(parts[4]) << 8) + std::stoi(parts[5]);
            return formatReply(COMMAND_OK, "PORT command successful");
        } catch (...) {
            return formatReply(SYNTAX_ERROR_PARAMS, "Invalid PORT values");
        }
    }

    if (verb == "PASV") {
        // Own the channel on the Session (not a local variable) so it
        // survives past this call, ready for the LIST/RETR/STOR that
        // arrives as a separate handleCommand() call afterward.
        session.dataChannel = std::make_unique<MockRDTChannel>();
        int port = session.dataChannel->bindPassive();
        if (port < 0) {
            session.dataChannel.reset();
            return formatReply(CANT_OPEN_DATA_CONN, "Could not open passive port");
        }

        session.passiveMode = true;
        session.dataPeerHost.clear();  // not used in passive mode
        session.dataPeerPort = port;   // the port WE bound and are listening on

        int p1 = port / 256;
        int p2 = port % 256;
        return formatReply(ENTERING_PASSIVE_MODE,
            "Entering Passive Mode (127,0,0,1," + std::to_string(p1) + "," + std::to_string(p2) + ")");
    }

    // TODO Fulfilled: Khởi tạo RDT Channel ngay tại đây để không làm phiền main.cpp
    if (verb == "LIST" || verb == "RETR" || verb == "STOR") {
        if (session.passiveMode) {
            // PASV already bound session.dataChannel and is sitting on
            // the port advertised to the client — reuse it as-is.
            if (!session.dataChannel) {
                return formatReply(CANT_OPEN_DATA_CONN, "Use PASV first");
            }
            // Block here until the client's data connection actually
            // shows up. Required for LIST/RETR, which only ever call
            // sendChunk() and would otherwise have no peer to send to
            // (STOR happens to discover the peer itself via its first
            // receiveChunk(), but waiting here too keeps all three
            // commands behaving the same way).
            if (!session.dataChannel->waitForPeer()) {
                return formatReply(CANT_OPEN_DATA_CONN, "Client did not connect to data port");
            }
        } else {
            // Active mode: open a fresh channel to the peer address/port
            // the client gave us via PORT.
            if (session.dataPeerHost.empty() || session.dataPeerPort <= 0) {
                return formatReply(CANT_OPEN_DATA_CONN, "Use PORT or PASV first");
            }
            session.dataChannel = std::make_unique<MockRDTChannel>();
            if (!session.dataChannel->open(session.dataPeerHost, session.dataPeerPort)) {
                session.dataChannel.reset();
                return formatReply(CANT_OPEN_DATA_CONN, "Could not open data connection");
            }
        }

        std::string reply;
        if (verb == "LIST") reply = handleLIST(args, session, *session.dataChannel);
        else if (verb == "RETR") reply = handleRETR(args, session, *session.dataChannel);
        else if (verb == "STOR") reply = handleSTOR(args, session, *session.dataChannel);

        session.dataChannel->close();
        session.dataChannel.reset();
        session.passiveMode = false; // one-shot: next transfer needs a fresh PASV/PORT
        return reply;
    }

    return formatReply(COMMAND_NOT_IMPLEMENTED, "Command not implemented: " + verb);
}

} // namespace ftp