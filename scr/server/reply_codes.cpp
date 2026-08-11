#include "../../include/server/reply_codes.h"

namespace ftp {

std::string formatReply(int code, const std::string& message) {
    return std::to_string(code) + " " + message + "\r\n";
}

} // namespace ftp
