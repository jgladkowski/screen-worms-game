#ifndef PLAYER_H
#define PLAYER_H

#include <cinttypes>
#include <cmath>

#include <string>

class Player {
public:
    Player(std::string name) : name(name), turn_direction(0), eliminated(false) {}

    void set_eliminated(bool eliminated)            { this->eliminated = eliminated; }
    void set_turn_direction(uint8_t turn_direction) { this->turn_direction = turn_direction; }
    void set_direction(int16_t direction)           { this->direction = direction; }
    void set_coords(double x, double y) {
        this->x = x;
        this->y = y;
    }

    void change_direction(int16_t delta) {
        this->direction += delta;
        if (this->direction < 0)
            this->direction += 360;
        this->direction %= 360;
    }
    void change_coords() {
        this->x += std::cos((double)this->direction * 3.1415 / 180.0);
        this->y += std::sin((double)this->direction * 3.1415 / 180.0);
    }

    bool        is_eliminated()              const { return eliminated; }
    uint8_t     get_turn_direction()         const { return turn_direction; }
    std::string get_name()                   const { return name; }
    std::pair<int32_t, int32_t> get_coords() const { return {std::floor(x), std::floor(y)}; }
private:
    std::string name;
    uint8_t turn_direction;

    bool eliminated;
    double x, y;
    int16_t direction;
};

#endif // PLAYER_H
