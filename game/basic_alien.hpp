#ifndef BASIC_ALIEN_HPP
#define BASIC_ALIEN_HPP

#include "alien.hpp"

class BasicAlien : public Alien {
public:
    BasicAlien();
    BasicAlien(int x, int y);
    ~BasicAlien();

    void draw(RGBMatrix *matrix) override;
    void move(int dx, int dy) override;

private:
    int horizontalDirection;
    int moveCounter;
};

#endif
