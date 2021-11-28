CC = g++
CFLAGS = -Wall -Wextra -O2 -std=c++17

all: address.h client.cpp client_datagram.cpp client_datagram.h client.h connected_client.h event.cpp event.h player.h screen_worms_client.cpp screen_worms_server.cpp server.cpp server_datagram.cpp server_datagram.h server.h utilities.cpp utilities.h
	$(CC) $(CFLAGS) utilities.cpp server_datagram.cpp client.cpp screen_worms_client.cpp -o screen-worms-client -lpthread
	$(CC) $(CFLAGS) utilities.cpp client_datagram.cpp event.cpp server.cpp screen_worms_server.cpp -o screen-worms-server -lpthread
clean:
	rm screen-worms-client
	rm screen-worms-server
