#pragma once
#include <string>
#include "session.h"
#include "rdt_interface.h"

namespace ftp {

// These need an IRDTChannel (Member 1's class, or MockRDTChannel for
// testing) that's already open()'d for this transfer — see the
// dispatcher TODO about how RETR/STOR get wired in.
std::string handleRETR(const std::string& args, Session& session, IRDTChannel& rdt);
std::string handleSTOR(const std::string& args, Session& session, IRDTChannel& rdt);

// TODO(you): STOU, APPE — same pattern as STOR (open ofstream, pump
// chunks from rdt.receiveChunk()), with STOU picking a unique
// filename instead of using the client's, and APPE opening with
// std::ios::app instead of std::ios::trunc.
// DELE and RNFR/RNTO don't touch the data channel — fine to put them
// in fs_handler.cpp instead if you'd rather group by "moves bytes"
// vs "metadata only".

} // namespace ftp
