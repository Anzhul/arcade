#ifndef LAUNCHER_ALIEN_HPP
#define LAUNCHER_ALIEN_HPP

#include "alien.hpp"

class LauncherAlien : public Alien {
public:
    LauncherAlien();
    LauncherAlien(int x, int y);
    ~LauncherAlien();

    void draw(RGBMatrix *matrix) override;
    void move(int dx, int dy) override;
    bool shouldShoot();
    void resetShotCooldown();

private:
    int horizontalDirection;
    int moveCounter;
    int shotCooldown;
    int anchorY;  // Y position to hold at
    bool anchored; // Whether we've reached our anchor point
};

#endif
