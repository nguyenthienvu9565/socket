#pragma once
#include <string>
#include "session.h"

namespace ftp {

// Parses one line from the TCP control channel and returns the
// server's reply, mutating `session` as needed (e.g. after USER/PASS,
// CWD, RNFR...). This is the single entry point client_session code
// in main.cpp calls once per received command line.
std::string handleCommand(const std::string& line, Session& session);

} // namespace ftp
