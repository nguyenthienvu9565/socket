#pragma once
#include <string>
#include "session.h"

namespace ftp {

std::string handleUSER(const std::string& args, Session& session);
std::string handlePASS(const std::string& args, Session& session);
std::string handleQUIT(Session& session);

} // namespace ftp
