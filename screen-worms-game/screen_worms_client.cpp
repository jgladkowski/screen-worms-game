#include <unistd.h>
#include <cinttypes>
#include <cstring>
#include <netinet/in.h>
#include <netdb.h>

#include <iostream>
#include <string>

#include "utilities.h"
#include "client.h"

namespace {
    constexpr uint32_t MAX_PLAYER_NAME_LENGTH = 20;

    void get_options(int argc, char **argv, std::string *player_name, std::string *port,
                     std::string *gui_server, std::string *gui_port) {
        int32_t option;
        while ((option = getopt(argc, argv, "n:p:i:r:")) != -1) {
            switch (option) {
            case 'n':
                *player_name = optarg;
                break;
            case 'i':
                *gui_server = optarg;
                break;
            case 'p':
                *port = optarg;
                break;
            case 'r':
                *gui_port = optarg;
                break;
            case '?':
            case ':':
                exit(1);
            }
        }
    }

    int prepare_gui_socket(std::string const& gui_server, std::string const& gui_port) {
        addrinfo addr_hints;
        memset(&addr_hints, 0, sizeof(addr_hints));
        addrinfo *addr_result;
        addr_hints.ai_socktype = SOCK_STREAM;
        addr_hints.ai_protocol = IPPROTO_TCP;
        if (getaddrinfo(gui_server.c_str(), gui_port.c_str(), &addr_hints, &addr_result) != 0) {
            std::cerr << "Error while getting GUI server address info!" << std::endl;
            exit(1);
        }

        // Prepare socket for communication with the gui server
        int gui_sock = socket(addr_result->ai_family, SOCK_STREAM, 0);
        if (gui_sock < 0) {
            std::cerr << "Error while creating GUI socket!" << std::endl;
            exit(1);
        }
        if (connect(gui_sock, addr_result->ai_addr, addr_result->ai_addrlen) < -1) {
            std::cerr << "Error while connecting to the GUI server!" << std::endl;
            exit(1);
        }
        return gui_sock;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "No game server provided!" << std::endl;
        exit(1);
    }

    // Program arguments and their default values
    std::string game_server = argv[1];
    std::string player_name = "";
    std::string port        = "2021";
    std::string gui_server  = "localhost";
    std::string gui_port    = "20210";
    get_options(argc, argv, &player_name, &port, &gui_server, &gui_port);
    if (port_number_from_string(port.c_str()) == 0 || port_number_from_string(gui_port.c_str()) == 0) {
        std::cerr << "Invalid port number!" << std::endl;
        return 1;
    }
    if (player_name.size() > MAX_PLAYER_NAME_LENGTH) {
        std::cerr << "Player name too long!" << std::endl;
        return 1;
    }

    // Getting server address
    addrinfo addr_hints;
    memset(&addr_hints, 0, sizeof(addr_hints));
    addrinfo *addr_result;
    addr_hints.ai_socktype = SOCK_DGRAM;
    addr_hints.ai_protocol = IPPROTO_UDP;
    if (getaddrinfo(game_server.c_str(), port.c_str(), &addr_hints, &addr_result) != 0) {
        std::cerr << "Error while getting server address info!" << std::endl;
        exit(1);
    }
    sockaddr_in6 server_address;
    server_address = *((sockaddr_in6 *) addr_result->ai_addr);

    // Prepare socket for communication with the server
    int sock = socket(server_address.sin6_family, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Error while creating socket!" << std::endl;
        exit(1);
    }
    sockaddr_in6 client_address;
    client_address.sin6_family = server_address.sin6_family;
    client_address.sin6_addr = in6addr_any;
    client_address.sin6_port = 0;
    if (bind(sock, (struct sockaddr *) &client_address,
        (socklen_t) sizeof(client_address)) < 0) {
        std::cerr << "Error while binding socket!" << std::endl;
        exit(1);
    }

    // Prepare socket for communication with the gui
    int gui_sock = prepare_gui_socket(gui_server, gui_port);

    Client client(player_name, sock, gui_sock, server_address);
    client.start();
}
