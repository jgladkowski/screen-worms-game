#ifndef UTILITIES_H
#define UTILITIES_H

#include <cinttypes>

#include <string>

bool is_correct_name(std::string const& name);

uint16_t port_number_from_string(char const *port_str);

uint32_t calculate_crc32(char const *buffer, uint32_t length);

class RandomGenerator {
public:
    RandomGenerator(uint32_t seed) : next_random(seed) {}
    uint32_t get_next_random();
private:
    uint32_t next_random;
};

#endif // UTILITIES_H
