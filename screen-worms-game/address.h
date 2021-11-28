#ifndef ADDRESS_H
#define ADDRESS_H

#include <cstring>
#include <netinet/in.h>

// Address class - a wrapper for sockaddr_in6
// Used for having addresses as keys in maps

class Address {
public:
    Address() = default;
    Address(sockaddr_in6 address) : address(address) {}

    sockaddr_in6 get_address() const { return address; }

    bool operator<(Address const& other) const {
        return memcmp((void *) &this->address, (void *) &other.address, sizeof(sockaddr_in6)) < 0;
    }
private:
    sockaddr_in6 address;
};

#endif // ADDRESS_H
