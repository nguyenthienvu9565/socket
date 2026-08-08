#pragma once
#include <string>
#include "session.h"

namespace ftp {

std::string handleCommand(const std::string& line, Session& session);

} // namespace ftp