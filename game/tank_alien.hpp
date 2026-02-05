#ifndef TANK_ALIEN_HPP
#define TANK_ALIEN_HPP

#include "alien.hpp"

class TankAlien : public Alien {
public:
    TankAlien();
    TankAlien(int x, int y);
    ~TankAlien();
    
    void draw(RGBMatrix *matrix) override;
};

#endif
