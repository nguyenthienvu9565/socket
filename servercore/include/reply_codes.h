#pragma once
#include <string>

namespace ftp {

// Standard 3-digit FTP reply codes used across this project.
// See Section 2.3 of the project spec for the full table.
enum ReplyCode : int {
    // 1xx - Positive Preliminary
    DATA_CONN_ALREADY_OPEN       = 125,
    FILE_STATUS_OK                = 150,

    // 2xx - Positive Completion
    COMMAND_OK                    = 200,
    SERVICE_READY                 = 220,
    GOODBYE                        = 221,
    TRANSFER_COMPLETE             = 226,
    ENTERING_PASSIVE_MODE         = 227,
    LOGIN_SUCCESSFUL               = 230,
    FILE_ACTION_OK                 = 250,
    PATHNAME_CREATED               = 257, // used by PWD/MKD to return the path

    // 3xx - Positive Intermediate
    USERNAME_OK_NEED_PASS         = 331,
    FILE_ACTION_PENDING            = 350, // RNFR waiting for RNTO

    // 4xx - Transient Negative
    SERVICE_UNAVAILABLE            = 421,
    CANT_OPEN_DATA_CONN             = 425,
    CONN_CLOSED_TRANSFER_ABORTED    = 426,
    FILE_UNAVAILABLE_TRANSIENT      = 450,

    // 5xx - Permanent Negative
    SYNTAX_ERROR                    = 500,
    SYNTAX_ERROR_PARAMS              = 501,
    COMMAND_NOT_IMPLEMENTED           = 502,
    NOT_LOGGED_IN                      = 530,
    FILE_UNAVAILABLE                    = 550,
};

// Formats "CODE message\r\n" the way the TCP control channel expects.
std::string formatReply(int code, const std::string& message);

} // namespace ftp
