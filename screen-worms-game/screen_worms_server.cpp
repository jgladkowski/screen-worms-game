#include <unistd.h>
#include <cstring>
#include <cinttypes>
#include <netinet/in.h>

#include <iostream>
#include <string>

#include "utilities.h"
#include "server.h"

namespace {
    // Constants
    constexpr uint32_t DEFAULT_PORT           = 2021;
    constexpr uint32_t DEFAULT_TURNING_SPEED  = 6;
    constexpr uint32_t DEFAULT_ROUNDS_PER_SEC = 50;
    constexpr uint32_t DEFAULT_WIDTH          = 640;
    constexpr uint32_t DEFAULT_HEIGHT         = 480;

    // Functions
    bool get_numerical_value(char const * option_string, uint32_t &numerical_value) {
        uint32_t option_length = strlen(option_string);
        for (uint32_t i = 0; i < option_length; i++)
            if (option_string[i] < '0' || option_string[i] > '9')
                return false;
        numerical_value = atoi(option_string);
        return true;
    }

    void get_options(int argc, char **argv, uint32_t &port, uint32_t &seed,
                     uint32_t &turning_speed, uint32_t &rounds_per_sec,
                     uint32_t &width, uint32_t &height) {
        int32_t option;
        while ((option = getopt(argc, argv, "p:s:t:v:w:h:")) != -1) {
            switch (option) {
            case 'p':
                port = port_number_from_string(optarg);
                break;
            case 's':
                if (!get_numerical_value(optarg, seed)) {
                    std::cerr << "Seed must be a number!" << std::endl;
                    exit(1);
                }
                break;
            case 't':
                if (!get_numerical_value(optarg, turning_speed)) {
                    std::cerr << "Turning speed must be a number!" << std::endl;
                    exit(1);
                }
                break;
            case 'v':
                if (!get_numerical_value(optarg, rounds_per_sec)) {
                    std::cerr << "Rounds per second must be a number!" << std::endl;
                    exit(1);
                }
                break;
            case 'w':
                if (!get_numerical_value(optarg, width)) {
                    std::cerr << "Width must be a number!" << std::endl;
                    exit(1);
                }
                break;
            case 'h':
                if (!get_numerical_value(optarg, height)) {
                    std::cerr << "Height must be a number!" << std::endl;
                    exit(1);
                }
                break;
            case ':':
            case '?':
                exit(1);
            }
        }
    }

    int prepare_socket(int port) {
        int sock = socket(AF_INET6, SOCK_DGRAM, 0);
        if (sock < 0) {
            std::cerr << "Error while creating socket!" << std::endl;
            exit(1);
        }
        sockaddr_in6 server_address;
        server_address.sin6_family = AF_INET6;
        server_address.sin6_addr = in6addr_any;
        server_address.sin6_port = htons(port);
        if (bind(sock, (struct sockaddr *) &server_address,
            (socklen_t) sizeof(server_address)) < 0) {
            std::cerr << "Error while binding socket!" << std::endl;
            exit(1);
        }
        return sock;
    }
}

int main(int argc, char **argv) {
    uint32_t port           = DEFAULT_PORT;
    uint32_t seed           = time(NULL);
    uint32_t turning_speed  = DEFAULT_TURNING_SPEED;
    uint32_t rounds_per_sec = DEFAULT_ROUNDS_PER_SEC;
    uint32_t width          = DEFAULT_WIDTH;
    uint32_t height         = DEFAULT_HEIGHT;
    get_options(argc, argv, port, seed, turning_speed,
                rounds_per_sec, width, height);

    if (port == 0) {
        std::cerr << "Invalid port number!" << std::endl;
        return 1;
    }
    if (width < 16 || width > 4096) {
        std::cerr << "Invalid width!" << std::endl;
        return 1;
    }
    if (height < 16 || height > 4096) {
        std::cerr << "Invalid height!" << std::endl;
        return 1;
    }
    if (turning_speed > 90) {
        std::cerr << "Turning speed too large!" << std::endl;
        return 1;
    }
    if (rounds_per_sec > 250) {
        std::cerr << "Too many rounds per second!" << std::endl;
        return 1;
    }

    int sock = prepare_socket(port);

    Server server(sock, seed, turning_speed, rounds_per_sec, width, height);
    server.start();
}
