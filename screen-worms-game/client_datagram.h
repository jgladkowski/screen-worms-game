#ifndef CLIENT_DATAGRAM_H
#define CLIENT_DATAGRAM_H

#include <string>

#include "address.h"

class ClientDatagram {
public:
    ClientDatagram(int sock);

    bool is_valid() const;
    
    Address     get_client_address()         const { return client_address; }
    uint64_t    get_session_id()             const { return session_id; }
    uint8_t     get_turn_direction()         const { return turn_direction; }
    uint32_t    get_next_expected_event_no() const { return next_expected_event_no; }
    std::string get_name()                   const { return name; }
private:
    uint32_t datagram_length;

    Address client_address;
    uint64_t session_id;
    uint8_t turn_direction;
    uint32_t next_expected_event_no;
    std::string name;
};

#endif // CLIENT_DATAGRAM_H
