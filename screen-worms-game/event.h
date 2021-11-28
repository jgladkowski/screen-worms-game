#ifndef EVENT_H
#define EVENT_H

#include <cinttypes>

#include <string>

class Event {
public:
    Event(uint32_t length) : length(length), buffer(new char[length]), write_ptr(buffer) {}

    uint32_t get_length() const { return length; }

    void put_string(std::string str);
    void put_uint32_t(uint32_t number);
    void put_uint8_t(uint8_t number);

    void calculate_crc32();
    void get_buffer(char *dst);
private:
    uint32_t length;
    char *buffer;
    char *write_ptr;
};

#endif // EVENT_H
