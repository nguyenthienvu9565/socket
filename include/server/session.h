#pragma once
#include <string>
#include <filesystem>

namespace ftp {

// One Session = one connected client's state. Created when a client
// connects, destroyed when it disconnects. Each client thread owns
// exactly one Session — no two threads touch the same Session, so it
// does NOT need its own mutex. (A *table* of all sessions, if you add
// one for logging/listing connected clients, DOES need a mutex — see
// the TODO at the bottom of this file.)
struct Session {
    int socketFd = -1;
    std::string clientId;              // e.g. "ip:port", for logging

    bool authenticated = false;
    std::string username;

    // Root directory this client is sandboxed to (e.g. "./ftp_root").
    // cwd must never be allowed to resolve outside this root — see
    // resolveSafePath() in fs_handler.cpp.
    std::filesystem::path rootDir;
    std::filesystem::path cwd;         // current working dir, relative to rootDir

    // Set by RNFR, consumed by RNTO. Empty = no rename pending.
    std::string renameFromPath;

    // Data-channel negotiation state (Active/Passive). Filled in by
    // your PORT/PASV handlers once you implement them; read by
    // transfer_handler.cpp when RETR/STOR actually need to move bytes.
    bool passiveMode = false;
    std::string dataPeerHost;          // for PORT (active) mode
    int dataPeerPort = -1;
};

// TODO(you): the spec (section 4.5) wants the server log to show a
// live "active session table" of connected clients. If you need that,
// add something like:
//
// class ClientRegistry {
// public:
//     void add(const std::string& clientId);
//     void remove(const std::string& clientId);
//     std::vector<std::string> list() const;
// private:
//     mutable std::mutex mutex_;
//     std::vector<std::string> clients_; // protected by mutex_
// };
//
// Every method must take the lock (std::lock_guard<std::mutex>)
// before touching clients_, since every client thread calls this
// concurrently — this is the actual "Concurrency Control" the spec
// is grading you on, not just spawning threads.

} // namespace ftp
