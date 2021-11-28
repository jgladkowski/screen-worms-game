#ifndef SERVER_DATAGRAM_H
#define SERVER_DATAGRAM_H

#include <cinttypes>
#include <netinet/in.h>

#include <string>
#include <vector>

constexpr uint32_t MAX_SERVER_DATAGRAM_LENGTH = 548;

class ServerDatagram {
public:
    ServerDatagram(int sock, sockaddr_in6 address);
    
    uint32_t get_game_id() const { return game_id; }
    bool     is_valid()    const { return valid; }

    void move_to_next_event();
    int get_next_event_info(uint8_t &event_type, uint32_t &event_no);
        // Returns the length of the event data in bytes
        // Returns -1 if the checksum is incorrect
        // Returns 0 if there is nothing more to read or if the length is incorrect (datagram ends prematurely)
                                                                      
    void read_new_game_event(uint32_t &maxx, uint32_t &maxy, std::vector<std::string> &players);
    void read_pixel_event(uint8_t &player_number, uint32_t &x, uint32_t &y);
    void read_player_eliminated_event(uint8_t &player_number);
private:
    char buffer[MAX_SERVER_DATAGRAM_LENGTH + 1]; // Buffer with datagram data
    uint32_t buffer_length;

    char *buffer_ptr;
    uint32_t bytes_left;
    uint32_t event_length; // Length of the event currently being investigated

    uint32_t game_id;
    bool valid;
};

#endif // SERVER_DATAGRAM_H
