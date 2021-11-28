#include <iostream>
#include <cstring>

#include "client_datagram.h"

namespace {
    constexpr uint32_t MAX_CLIENT_NAME_LENGTH     = 20;
    constexpr uint8_t  MAX_TURN_DIRECTION         =  2;

    constexpr uint32_t MIN_CLIENT_DATAGRAM_LENGTH = 13;
    constexpr uint32_t MAX_CLIENT_DATAGRAM_LENGTH = MIN_CLIENT_DATAGRAM_LENGTH + MAX_CLIENT_NAME_LENGTH;
}

bool ClientDatagram::is_valid() const {
    return this->datagram_length >= MIN_CLIENT_DATAGRAM_LENGTH &&
           this->datagram_length <= MAX_CLIENT_DATAGRAM_LENGTH &&
           turn_direction <= MAX_TURN_DIRECTION;
}

ClientDatagram::ClientDatagram(int sock) {
    sockaddr_in6 address;
    socklen_t addrlen = sizeof(address);
    char buffer[MAX_CLIENT_DATAGRAM_LENGTH + 1];
    memset(buffer, 0, MAX_CLIENT_DATAGRAM_LENGTH + 1);
    int rcv_length = recvfrom(sock, buffer, MAX_CLIENT_DATAGRAM_LENGTH + 1, 0,
                              (sockaddr *) &address, &addrlen);
    if (rcv_length < 0) {
        std::cerr << "Error on datagram from client socket!" << std::endl;
        exit(1);
    }

    this->session_id             = be64toh(*((uint64_t *) buffer));
    this->turn_direction         = *((uint8_t *) (buffer + 8));
    this->next_expected_event_no = ntohl(*((uint32_t *) (buffer + 9)));
    this->name                   = buffer + 13;
    this->client_address         = Address(address);
    this->datagram_length        = rcv_length;
}
