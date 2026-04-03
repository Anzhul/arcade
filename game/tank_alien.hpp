#ifndef TANK_ALIEN_HPP
#define TANK_ALIEN_HPP

#include "alien.hpp"

class TankAlien : public Alien {
public:
    TankAlien();
    TankAlien(int x, int y);
    ~TankAlien();

    void draw(RGBMatrix *matrix) override;
    void move(int dx, int dy) override;
    void setBehavior(int mode) { behaviorMode = mode; }
    int getBehavior() const { return behaviorMode; }

private:
    int frameCounter;
    // Behavior: 0=default (straight down), 1=guard (anchor+patrol), 2=patrol (left-right while descending), 3=carousel (move right, wrap to left)
    int behaviorMode;
    int horizontalDirection;
    int moveCounter;
    int anchorY;
    bool anchored;
};

#endif
