#ifndef CLIENT_H
#define CLIENT_H

#include <cinttypes>

#include <string>
#include <chrono>

#include "address.h"

class ConnectedClient {
public:
    ConnectedClient(Address client_address, uint64_t session_id, std::string name)
        : client_address(client_address), session_id(session_id), name(name),
        in_game(name.size() == 0), connection_time(std::chrono::steady_clock::now()) {}

    void join_game() { in_game = true; }
    void disconnect_from_game() { 
        if (name.size() > 0)
            in_game = false;
    }

    std::chrono::time_point<std::chrono::steady_clock> get_connection_time() const { return connection_time; }
    void update_connection_time() { connection_time = std::chrono::steady_clock::now(); }

    Address     get_client_address() const { return client_address; }
    uint64_t    get_session_id()     const { return session_id; }
    std::string get_name()           const { return name; }
    bool        is_in_game()         const { return in_game; }
private:
    Address client_address;
    uint64_t session_id;
    std::string name;
    bool in_game;
    std::chrono::time_point<std::chrono::steady_clock> connection_time;
};

#endif // CLIENT_H
