#pragma once
#include <string>
#include "../session.h"
#include "../rdt_interface.h" // Thêm để nhận diện IRDTChannel

namespace ftp {

std::string handlePWD(Session& session);
std::string handleCWD(const std::string& args, Session& session);
std::string handleCDUP(Session& session);
std::string handleMKD(const std::string& args, Session& session);
std::string handleRMD(const std::string& args, Session& session);

// Đã cập nhật theo yêu cầu của spec: Data Channel cho LIST
std::string handleLIST(const std::string& args, Session& session, IRDTChannel& rdt);

std::string handleNLST(const std::string& args, Session& session);
std::string handleSIZE(const std::string& args, Session& session);
std::string handleMDTM(const std::string& args, Session& session);

} // namespace ftp