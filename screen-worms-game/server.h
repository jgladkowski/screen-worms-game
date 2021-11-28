#ifndef SERVER_H
#define SERVER_H

#include <cinttypes>
#include <netinet/in.h>
#include <poll.h>

#include <string>
#include <vector>
#include <set>
#include <map>
#include <unordered_set>
#include <thread>
#include <condition_variable>

#include "utilities.h"
#include "address.h"
#include "event.h"
#include "player.h"
#include "connected_client.h"
#include "client_datagram.h"

class Server {
public:
    void start();

    Server(uint32_t sock, uint32_t seed, uint32_t turning_speed,
           uint32_t rounds_per_sec, uint32_t width, uint32_t height) :
        sock(sock), turning_speed(turning_speed),
        round_time(1000000000 / rounds_per_sec), // nanoseconds per round
        width(width), height(height),
        game_active(false), active_players(0),
        generator(RandomGenerator(seed)) {}

private:
    void read_one_datagram();
    void connect_new_client(ClientDatagram const& datagram);
    void disconnect_client(Address const& client_address);
    void send_events_to_client(uint32_t next_expected_event_no, ConnectedClient const& client);

    void push_new_game_event();
    void push_pixel_event(uint8_t player_number, uint32_t x, uint32_t y);
    void push_player_eliminated_event(uint8_t player_number);
    void push_game_over_event();
    
    void make_game_start();
    void make_one_round();
    bool make_one_round_with_clock();

    uint32_t sock;

    uint32_t turning_speed;
    uint32_t round_time;
    uint32_t width;
    uint32_t height;

    uint32_t game_id;
    bool game_active;
    uint8_t active_players;

    RandomGenerator generator;

    std::vector<Event> events_list;
    std::map<std::string, Player> current_players;

    std::map<Address, ConnectedClient> connected_clients;
    std::unordered_set<std::string> taken_names;

    std::set<std::pair<int32_t, int32_t>> eaten_pixels;

    std::condition_variable cond;
    std::mutex mutex;
};

#endif // SERVER_H
