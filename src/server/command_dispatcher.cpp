#include "../../include/server/command_dispatcher.h"
#include "../../include/server/reply_codes.h"
#include "../../include/server/handlers/auth_handler.h"
#include "../../include/server/handlers/fs_handler.h"
#include "../../include/server/handlers/transfer_handler.h" 
#include "../../include/server/real_rdt.h"   
#include "../../include/core/rdt.h"                  
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
    if (verb == "SIZE") return handleSIZE(args, session);
    if (verb == "MDTM") return handleMDTM(args, session);

    if (verb == "PORT") {
        auto parts = splitCommaArgs(args);
        if (parts.size() != 6) return formatReply(SYNTAX_ERROR_PARAMS, "Invalid PORT signature");
        try {
            session.passiveMode = false;
            session.dataChannel.reset(); 
            session.dataPeerHost = parts[0] + "." + parts[1] + "." + parts[2] + "." + parts[3];
            session.dataPeerPort = (std::stoi(parts[4]) << 8) + std::stoi(parts[5]);
            return formatReply(COMMAND_OK, "PORT command successful");
        } catch (...) {
            return formatReply(SYNTAX_ERROR_PARAMS, "Invalid PORT values");
        }
    }

    if (verb == "PASV") {
        session.dataChannel = std::make_unique<RealRDTChannel>();
        int port = session.dataChannel->bindPassive();
        if (port < 0) {
            session.dataChannel.reset();
            return formatReply(CANT_OPEN_DATA_CONN, "Could not open passive port");
        }

        session.passiveMode = true;
        session.dataPeerHost.clear();  
        session.dataPeerPort = port;   

        int p1 = port / 256;
        int p2 = port % 256;
        return formatReply(ENTERING_PASSIVE_MODE,
            "Entering Passive Mode (127,0,0,1," + std::to_string(p1) + "," + std::to_string(p2) + ")");
    }

    if (verb == "LIST" || verb == "NLST" || verb == "RETR" || verb == "STOR") {
        if (session.passiveMode) {
            if (!session.dataChannel) {
                return formatReply(CANT_OPEN_DATA_CONN, "Use PASV first");
            }
        } else { // Active Mode
            if (session.dataPeerHost.empty() || session.dataPeerPort <= 0) {
                return formatReply(CANT_OPEN_DATA_CONN, "Use PORT or PASV first");
            }
        }

        std::string startReply = formatReply(FILE_STATUS_OK, "Opening data connection");
        std::vector<char> startPayload(startReply.begin(), startReply.end());
        rdt_send_buffer(session.socketFd, session.clientControlAddr, startPayload);

        // 2. SAU ĐÓ SERVER MỚI CHỜ KẾT NỐI DATA CHANNEL TỪ CLIENT
        if (session.passiveMode) {
            // Có thể bọc thêm cơ chế timeout như tôi đã hướng dẫn trước đó
            if (!session.dataChannel->waitForPeer()) {
                session.dataChannel->close();
                session.dataChannel.reset();
                session.passiveMode = false;
                return formatReply(CANT_OPEN_DATA_CONN, "Client connection timeout on data port");
            }
        } else {
            session.dataChannel = std::make_unique<RealRDTChannel>();
            if (!session.dataChannel->open(session.dataPeerHost, session.dataPeerPort)) {
                session.dataChannel.reset();
                return formatReply(CANT_OPEN_DATA_CONN, "Could not open data connection");
            }
        }

        std::string reply;
        if (verb == "LIST") reply = handleLIST(args, session, *session.dataChannel);
        else if (verb == "NLST") reply = handleNLST(args, session, *session.dataChannel);
        else if (verb == "RETR") reply = handleRETR(args, session, *session.dataChannel);
        else if (verb == "STOR") {
            if (!session.passiveMode) {
                // [Reverse Ping] Gửi 1 byte mồi cho Client để lộ tọa độ Data Port
                std::vector<char> ping = {'P'};
                session.dataChannel->sendBuffer(ping); 
            }
            reply = handleSTOR(args, session, *session.dataChannel);
        }

        session.dataChannel->close();
        session.dataChannel.reset();
        session.passiveMode = false; 
        return reply;
    }

    return formatReply(COMMAND_NOT_IMPLEMENTED, "Command not implemented: " + verb);
}

} // namespace ftp