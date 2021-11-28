#include <cstring>

#include "utilities.h"
#include "server_datagram.h"

namespace {
    constexpr uint32_t MIN_SERVER_DATAGRAM_LENGTH = 4;
}

ServerDatagram::ServerDatagram(int sock, sockaddr_in6 address) {
    memset(buffer, 0, MAX_SERVER_DATAGRAM_LENGTH + 1);

    socklen_t address_length = sizeof(address);
    this->buffer_length = recvfrom(sock, this->buffer, MAX_SERVER_DATAGRAM_LENGTH + 1, 0,
                                    (sockaddr *) &address, &address_length);
    if (this->buffer_length < MIN_SERVER_DATAGRAM_LENGTH || this->buffer_length > MAX_SERVER_DATAGRAM_LENGTH) {
        this->valid = false;
        return;
    }

    this->game_id = ntohl(*((uint32_t *) buffer));
    this->valid = true;
    
    this->buffer_ptr = buffer + 4;
    this->bytes_left = this->buffer_length - 4;
}

void ServerDatagram::move_to_next_event() {
    this->buffer_ptr += this->event_length;
    this->bytes_left -= this->event_length;
}

int ServerDatagram::get_next_event_info(uint8_t &event_type, uint32_t &event_no) {
    // Checking if the datagram ends prematurely 
    if (this->bytes_left < 4)
        return 0;
    this->event_length = ntohl(*((uint32_t *) (this->buffer_ptr))) + 8; // Adding 8 to account for len and crc32
    if (this->bytes_left < this->event_length)
        return 0;

    // Checking whether the checksum is correct
    uint32_t crc32_local = calculate_crc32(this->buffer_ptr, this->event_length - 4);
    uint32_t crc32_from_datagram = ntohl(*((uint32_t *) (this->buffer_ptr + this->event_length - 4)));
    if (crc32_local != crc32_from_datagram)
        return -1;

    // Extracting event_no and event_type from the datagram
    event_no   = ntohl(*((uint32_t *) (this->buffer_ptr + 4)));
    event_type = *((uint8_t *) (this->buffer_ptr + 8));
    return this->event_length;
}

void ServerDatagram::read_new_game_event(uint32_t &maxx, uint32_t &maxy, std::vector<std::string> &players) {
    maxx = ntohl(*((uint32_t *) (buffer_ptr + 9)));
    maxy = ntohl(*((uint32_t *) (buffer_ptr + 13)));
    players.clear();

    uint32_t players_data_length = this->event_length - 21;
    uint32_t data_considered = 0;
    char *players_data_ptr = this->buffer_ptr + 17;
    while (data_considered < players_data_length) {
        std::string name = players_data_ptr;
        players.push_back(name);

        data_considered += name.size() + 1;
        players_data_ptr += name.size() + 1;
    }
}

void ServerDatagram::read_pixel_event(uint8_t &player_number, uint32_t &x, uint32_t &y) {
    player_number = *((uint8_t *) (buffer_ptr + 9));
    x = ntohl(*((uint32_t *) (buffer_ptr + 10)));
    y = ntohl(*((uint32_t *) (buffer_ptr + 14)));
}

void ServerDatagram::read_player_eliminated_event(uint8_t &player_number) {
    player_number = *((uint8_t *) (buffer_ptr + 9));
}
