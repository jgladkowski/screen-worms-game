#include <sys/timerfd.h>
#include <unistd.h>
#include <poll.h>

#include <thread>
#include <chrono>
#include <iostream>

#include "server.h"

namespace {
    constexpr int64_t DISCONNECT_TIME_MILIS = 2000;

    constexpr uint32_t MAX_SERVER_MSG_SIZE  = 548;
    constexpr uint32_t MAX_NO_OF_PLAYERS    = 25;
}

/* ------------------- Server logic ------------------------ */

void Server::connect_new_client(ClientDatagram const& datagram) {
    Address client_address = datagram.get_client_address();
    uint64_t session_id    = datagram.get_session_id();
    std::string name       = datagram.get_name();

    // If the address is already connected
    if (connected_clients.find(client_address) != connected_clients.end()) {
        uint64_t prev_session_id = connected_clients.at(client_address).get_session_id();
        // If session_id is too large then establish new connection
        if (session_id > prev_session_id) {
            this->disconnect_client(client_address); // Now that the previous connection is gone
            this->connect_new_client(datagram);      // try connecting once again
        }
        return;
    }
    else if (name.size() > 0 && taken_names.count(name) > 0) // If the address is not connected and
        return;                                              // the name is taken then ignore the datagram

    // Connecting new client
    if (connected_clients.size() < MAX_NO_OF_PLAYERS) {
        connected_clients.insert({client_address, ConnectedClient(client_address, session_id, name)});
        if (name.size() > 0)
            taken_names.insert(name);
    }
}

void Server::disconnect_client(Address const& client_address) {
    if (connected_clients.count(client_address) > 0) {
        ConnectedClient client = connected_clients.at(client_address);
        std::string name = client.get_name();
        connected_clients.erase(client_address);
        if (name.size() > 0)
            taken_names.erase(name);
        if (client.is_in_game() && name.size() > 0) {
            current_players.erase(name);
            cond.notify_all();
        }
    }
}

void Server::send_events_to_client(uint32_t next_expected_event_no, ConnectedClient const& client) {
    sockaddr_in6 address = client.get_client_address().get_address();
    char buffer[MAX_SERVER_MSG_SIZE];
    memset(buffer, 0, MAX_SERVER_MSG_SIZE);

    uint32_t length = 4;
    uint32_t game_id_big_endian = htonl(game_id);
    memcpy((void *) buffer, (void *) &game_id_big_endian, 4);

    for (uint32_t i = next_expected_event_no; i <= events_list.size(); i++) {
        if (i == events_list.size() || length + events_list[i].get_length() > MAX_SERVER_MSG_SIZE) {
            if (length > 4)
                sendto(sock, buffer, length, 0, (sockaddr *) &address, (socklen_t) sizeof(address));
            length = 4;
        }
        if (i < events_list.size()) {
            events_list[i].get_buffer(buffer + length);
            length += events_list[i].get_length();
        }
    }
}

void Server::read_one_datagram() {
    int64_t poll_wait_time = -1;
    auto current_time = std::chrono::steady_clock::now();

    {
        std::unique_lock<std::mutex> lk(mutex);
        for (auto it = connected_clients.begin(); it != connected_clients.end(); ) {
            int64_t milis = std::chrono::duration_cast<std::chrono::milliseconds>(
                current_time - it->second.get_connection_time()).count();
            if (milis >= DISCONNECT_TIME_MILIS) {
                auto it_copy = it;
                it_copy++;
                disconnect_client(it->second.get_client_address());
                it = it_copy;
            }
            else {
                if (milis >= poll_wait_time)
                    poll_wait_time = DISCONNECT_TIME_MILIS - milis;
                it++;
            }
        }
    }

    pollfd poll_fd;
    poll_fd.fd = sock;
    poll_fd.events = POLLIN;
    poll_fd.revents = 0;
    poll(&poll_fd, 1, poll_wait_time);

    if (poll_fd.revents & POLLIN) {
        poll_fd.revents = 0;
        ClientDatagram datagram(sock);
        if (datagram.is_valid()) {
            std::unique_lock<std::mutex> lk(mutex);
            this->connect_new_client(datagram);
            if (connected_clients.count(datagram.get_client_address()) > 0) {
                ConnectedClient &client = connected_clients.at(datagram.get_client_address());
                client.update_connection_time();

                // If data doesn't match then ignore the datagram
                if (datagram.get_session_id() < client.get_session_id() || datagram.get_name() != client.get_name())
                    return;

                // Try to join the game that is about to start
                if (!game_active && !client.is_in_game() && datagram.get_turn_direction() > 0) {
                    current_players.insert({client.get_name(), Player(client.get_name())});
                    client.join_game();
                    this->active_players++;
                    cond.notify_all();
                }

                // Update turn_direction
                if (current_players.count(client.get_name()) > 0) {
                    Player &player = current_players.at(client.get_name());
                    player.set_turn_direction(datagram.get_turn_direction());
                }

                // Send events
                this->send_events_to_client(datagram.get_next_expected_event_no(), client);
            }
        }
    }
}

/* ---------------------- Adding new events ----------------------- */

void Server::push_new_game_event() {
    uint32_t length = 21;
    for (auto const& player : current_players)
        length += player.second.get_name().size() + 1;
    Event event(length);
    event.put_uint32_t(length - 8);
    event.put_uint32_t(events_list.size());
    event.put_uint8_t(0);
    event.put_uint32_t(this->width);
    event.put_uint32_t(this->height);
    for (auto const& player : current_players)
        event.put_string(player.second.get_name());
    event.calculate_crc32();
    events_list.push_back(event);
}

void Server::push_pixel_event(uint8_t player_number, uint32_t x, uint32_t y) {
    Event event(22);
    event.put_uint32_t(14);
    event.put_uint32_t(events_list.size());
    event.put_uint8_t(1);
    event.put_uint8_t(player_number);
    event.put_uint32_t(x);
    event.put_uint32_t(y);
    event.calculate_crc32();
    events_list.push_back(event);
}

void Server::push_player_eliminated_event(uint8_t player_number) {
    Event event(14);
    event.put_uint32_t(6);
    event.put_uint32_t(events_list.size());
    event.put_uint8_t(2);
    event.put_uint8_t(player_number);
    event.calculate_crc32();
    events_list.push_back(event);
}

void Server::push_game_over_event() {
    Event event(13);
    event.put_uint32_t(5);
    event.put_uint32_t(events_list.size());
    event.put_uint8_t(3);
    event.calculate_crc32();
    events_list.push_back(event);
}

/* --------------------------- Game logic -------------------------- */

void Server::make_game_start() {
    uint8_t player_no = 0;
    
    for (auto &string_and_player : current_players) {
        Player &player = string_and_player.second;
        player.set_eliminated(false);
        player.set_coords((this->generator.get_next_random() % width) + 0.5,
                        (this->generator.get_next_random() % height) + 0.5);
        player.set_direction(this->generator.get_next_random() % 360);
        std::pair<int32_t, int32_t> pixel = player.get_coords();

        if (eaten_pixels.count(pixel) > 0) {
            this->push_player_eliminated_event(player_no);
            player.set_eliminated(true);
            this->active_players--;
        }
        else {
            eaten_pixels.insert(pixel);
            this->push_pixel_event(player_no, pixel.first, pixel.second);
        }
        player_no++;
    }
}

void Server::make_one_round() {
    uint8_t player_no = 0;

    for (auto &string_and_player : current_players) {
        Player &player = string_and_player.second;
        if (!player.is_eliminated()) {

            std::pair<int32_t, int32_t> pixel_before = player.get_coords();

            if (player.get_turn_direction() == 1)
                player.change_direction(turning_speed);
            else if (player.get_turn_direction() == 2)
                player.change_direction(-turning_speed);
            player.change_coords();

            std::pair<int32_t, int32_t> pixel_after = player.get_coords();
            if (pixel_before != pixel_after) {
                if (pixel_after.first < 0 || pixel_after.second < 0 ||
                    (uint32_t)pixel_after.first >= width || (uint32_t)pixel_after.second >= height ||
                    eaten_pixels.count(pixel_after) > 0) {
                    this->push_player_eliminated_event(player_no);
                    player.set_eliminated(true);
                    this->active_players--;
                }
                else {
                    eaten_pixels.insert(pixel_after);
                    this->push_pixel_event(player_no, pixel_after.first, pixel_after.second);
                }
            }
            
        }

        player_no++;
    }
}

bool Server::make_one_round_with_clock() {
    // Creating timer file descriptor
    int timer_fd;
    if ((timer_fd = timerfd_create(CLOCK_REALTIME, 0)) < 0) {
        std::cerr << "Error while creating timerfd!" << std::endl;
        exit(1);
    }

    // Preparing poll
    pollfd poll_fd;
    poll_fd.fd = timer_fd;
    poll_fd.events = POLLIN;
    poll_fd.revents = 0;

    // Setting the timer to round_time
    itimerspec timer_length;
    timer_length.it_interval.tv_sec = 0;
    timer_length.it_interval.tv_nsec = 0;
    timer_length.it_value.tv_sec = 0;
    timer_length.it_value.tv_nsec = this->round_time;
    if (timerfd_settime(timer_fd, 0, &timer_length, NULL) < 0) {
        std::cerr << "Error while setting round time!" << std::endl;
        exit(1);
    }

    {
        std::unique_lock<std::mutex> lk(mutex);
        this->make_one_round();
        if (this->active_players < 2)
            return false;
    }

    // Remaining round_time
    if (poll(&poll_fd, 1, -1) < 0) {
        std::cerr << "Error in poll!" << std::endl;
        exit(1);
    }
    close(timer_fd);
    return true;
}

/* ---------------------- Starting server ------------------------ */

void Server::start() {
    std::thread reading_datagrams_thread([this]() {
        while (1)
            this->read_one_datagram();
    });

    while (1) {
        // Game start
        {
            std::unique_lock<std::mutex> lk(mutex);
            cond.wait(lk, [this]() {
            return this->taken_names.size() >= 2 &&
                   this->taken_names.size() == this->current_players.size();
            });
            this->events_list.clear();
            this->game_id = this->generator.get_next_random();
            this->game_active = true;
            this->eaten_pixels.clear();

            this->push_new_game_event();
            this->make_game_start();
        }

        // Game loop
        while (this->make_one_round_with_clock());

        // Game end
        std::unique_lock<std::mutex> lk(mutex);
        this->push_game_over_event();
        this->eaten_pixels.clear();
        this->current_players.clear();
        this->game_active = false;
        this->active_players = 0;
        for (auto &addr_and_client : connected_clients) {
            ConnectedClient &client = addr_and_client.second;
            client.disconnect_from_game();
        }
    }

    reading_datagrams_thread.join();
}
