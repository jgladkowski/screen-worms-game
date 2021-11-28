#ifndef CLIENT_H
#define CLIENT_H

#include <unistd.h>
#include <netinet/in.h>

#include <string>
#include <vector>
#include <mutex>

class Client {
public:
    Client(std::string const& name, int game_server_sock, int gui_sock, sockaddr_in6 const &game_server_address);
    ~Client() { delete[] datagram_to_send; }
    void start();
private:
    void send_datagram();
    void send_datagram_30ms();
    void receive_datagram();
    void get_data_from_gui();

    void handle_new_game_event(uint32_t game_id, uint32_t maxx, uint32_t maxy);
    void handle_pixel_event(uint32_t x, uint32_t y, uint8_t player_no);
    void handle_player_eliminated_event(uint8_t player_no);

    char *datagram_to_send;
    uint32_t datagram_to_send_length;

    int game_server_sock;
    int gui_sock;
    sockaddr_in6 game_server_address;

    bool game_in_progress;
    uint32_t game_id;
    std::vector<std::string> player_names;

    uint8_t turn_direction;
    uint32_t next_expected_event_no;

    std::mutex mutex;
    std::string current_data_from_gui;
};

#endif // CLIENT_H
