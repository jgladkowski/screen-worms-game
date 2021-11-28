#include <poll.h>
#include <sys/timerfd.h>
#include <cstring>

#include <iostream>
#include <thread>

#include "server_datagram.h"
#include "client.h"

namespace {
    constexpr uint32_t MIN_DATAGRAM_LENGTH = 13;
}

/* ------------------------- Sending datagrams to game server -------------------------- */

void Client::send_datagram() {
    std::unique_lock<std::mutex> lk(this->mutex);

    // Fill the datagram with data that can change
    uint32_t next_expected_event_no_network = htonl(this->next_expected_event_no);
    memcpy((void *) (this->datagram_to_send + 8), (void *) &this->turn_direction, 1);
    memcpy((void *) (this->datagram_to_send + 9), (void *) &next_expected_event_no_network, 4);

    // Send the datagram to the game server
    if (sendto(this->game_server_sock, this->datagram_to_send, this->datagram_to_send_length, 0,
        (sockaddr *) &this->game_server_address, (socklen_t) sizeof(this->game_server_address)) < 0) {
        std::cerr << "Error while sending the datagram to the server!" << std::endl;
        exit(1);
    }
}

void Client::send_datagram_30ms() {
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

    // Setting the timer to 30ms
    itimerspec timer_length;
    timer_length.it_interval.tv_sec = 0;
    timer_length.it_interval.tv_nsec = 0;
    timer_length.it_value.tv_sec = 0;
    timer_length.it_value.tv_nsec = 30000000; // 30ms
    if (timerfd_settime(timer_fd, 0, &timer_length, NULL) < 0) {
        std::cerr << "Error while settings 30ms time!" << std::endl;
        exit(1);
    }

    // Sending to the server
    this->send_datagram();

    // Remaining 30ms
    if (poll(&poll_fd, 1, -1) < 0) {
        std::cerr << "Error in poll!" << std::endl;
        exit(1);
    }
    close(timer_fd);
}

/* ------------------- Getting data from the game server -------------------- */

void Client::handle_new_game_event(uint32_t game_id, uint32_t maxx, uint32_t maxy) {
    this->game_id = game_id;
    this->game_in_progress = true;
    this->next_expected_event_no = 1;

    std::string message_to_gui = "NEW_GAME " + std::to_string(maxx) + " " + std::to_string(maxy);
    for (std::string const& name : this->player_names)
        message_to_gui.append(" " + name);
    message_to_gui.append("\n");

    size_t bytes_sent = 0;
    while (bytes_sent < message_to_gui.size())
        bytes_sent += write(this->gui_sock, message_to_gui.c_str() + bytes_sent, message_to_gui.size() - bytes_sent);
}

void Client::handle_pixel_event(uint32_t x, uint32_t y, uint8_t player_no) {
    if (player_no >= player_names.size()) {
        std::cerr << "Invalid player number!" << std::endl;
        exit(1);
    }
    std::string message_to_gui = "PIXEL " + std::to_string(x) + " " + std::to_string(y) + " " + player_names[player_no] + "\n";
    this->next_expected_event_no++;

    size_t bytes_sent = 0;
    while (bytes_sent < message_to_gui.size())
        bytes_sent += write(this->gui_sock, message_to_gui.c_str() + bytes_sent, message_to_gui.size() - bytes_sent);
}

void Client::handle_player_eliminated_event(uint8_t player_no) {
    if (player_no >= player_names.size()) {
        std::cerr << "Invalid player number!" << std::endl;
        exit(1);
    }
    std::string message_to_gui = "PLAYER_ELIMINATED " + player_names[player_no] + "\n";
    this->next_expected_event_no++;

    size_t bytes_sent = 0;
    while (bytes_sent < message_to_gui.size())
        bytes_sent += write(this->gui_sock, message_to_gui.c_str() + bytes_sent, message_to_gui.size() - bytes_sent);
}

void Client::receive_datagram() {
    // Reading datagram from the game server
    ServerDatagram server_datagram(this->game_server_sock, this->game_server_address);
    uint8_t event_type;
    uint32_t event_no;

    std::unique_lock<std::mutex> lk(mutex);

    // If there is no active game then try to join one
    if (!game_in_progress && 
        server_datagram.get_next_event_info(event_type, event_no) > 0 &&
        server_datagram.get_game_id() != game_id &&
        event_type == 0) {
        uint32_t maxx, maxy;
        server_datagram.read_new_game_event(maxx, maxy, this->player_names);
        this->handle_new_game_event(server_datagram.get_game_id(), maxx, maxy);
        server_datagram.move_to_next_event();
    }

    if (game_in_progress && server_datagram.get_game_id() == game_id) {
        int ret;
        while ((ret = server_datagram.get_next_event_info(event_type, event_no)) > 0) {
            if (event_no == this->next_expected_event_no) {
                uint32_t x, y;
                uint8_t player_no;
                switch (event_type) {
                case 0:
                    break; // Just ignore
                case 1:
                    server_datagram.read_pixel_event(player_no, x, y);
                    this->handle_pixel_event(x, y, player_no);
                    break;
                case 2:
                    server_datagram.read_player_eliminated_event(player_no);
                    this->handle_player_eliminated_event(player_no);
                    break;
                case 3:
                    game_in_progress = false;
                    next_expected_event_no = 0;
                    break;
                default:
                    std::cerr << "Received invalid event from the server!" << std::endl;
                    exit(1);
                }
            }
            server_datagram.move_to_next_event();
        }
    }
}

/* -------------------------- Getting data from gui ------------------------ */

void Client::get_data_from_gui() {
    char buffer[40];
    memset(buffer, 0, 40);

    int32_t received_length = read(gui_sock, buffer, 40 - current_data_from_gui.size());
    if (received_length < 0) {
        std::cerr << "Error receiving data from the GUI server!" << std::endl;
        exit(1);
    }

    std::unique_lock<std::mutex> lk(mutex);

    current_data_from_gui.append(buffer);
    if (current_data_from_gui.size() >= 40)
        current_data_from_gui.size();

    size_t newline_pos = current_data_from_gui.find_first_of('\n');
    while (newline_pos != std::string::npos) {
        std::string message = current_data_from_gui.substr(0, newline_pos);
        if (message == "RIGHT_KEY_DOWN")
            turn_direction = 1;
        else if (message == "LEFT_KEY_DOWN")
            turn_direction = 2;
        else if (message == "RIGHT_KEY_UP" || message == "LEFT_KEY_UP")
            turn_direction = 0;

        current_data_from_gui = current_data_from_gui.substr(newline_pos + 1);
        newline_pos = current_data_from_gui.find_first_of('\n');
    }
}

/* --------------------------- Initializing client ---------------------------- */

void Client::start() {
    std::thread receiving_datagrams_thread([this]() {
        while (1)
            this->receive_datagram();
    });
    std::thread receiving_data_from_gui_thread([this]() {
        while (1)
            this->get_data_from_gui();
    });
    while (1)
        this->send_datagram_30ms();
    receiving_datagrams_thread.join();
    receiving_data_from_gui_thread.join();
}

Client::Client(std::string const& name, int game_server_sock, int gui_sock, sockaddr_in6 const &game_server_address) {
    // Preparing the template for datagrams to game server
    this->datagram_to_send_length = MIN_DATAGRAM_LENGTH + name.size();
    this->datagram_to_send= new char[this->datagram_to_send_length];
    auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
    uint64_t session_id = htobe64(std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count());
    memset(datagram_to_send, 0, this->datagram_to_send_length);
    memcpy((void *) datagram_to_send, (void *) &session_id, 8);
    memcpy((void *) (datagram_to_send + 13), (void *) name.c_str(), name.size());

    // Initialize the variables
    this->game_server_sock    = game_server_sock;
    this->gui_sock            = gui_sock;
    this->game_server_address = game_server_address;

    this->game_in_progress       = false;
    this->turn_direction         = 0;
    this->next_expected_event_no = 0;
    this->current_data_from_gui  = "";
}
