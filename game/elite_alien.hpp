#ifndef ELITE_ALIEN_HPP
#define ELITE_ALIEN_HPP

#include "alien.hpp"

class EliteAlien : public Alien {
public:
    EliteAlien();
    EliteAlien(int x, int y);
    ~EliteAlien();
    
    void draw(RGBMatrix *matrix) override;
    void move(int dx, int dy) override;
    void takeDamage(int damage) override;
    bool shouldShoot();
    void resetShotCooldown();
    int getShield() const;

private:
    int horizontalDirection;
    int movePattern;
    int moveCounter;
    int shotCooldown;
    int shield;         // Shield HP (absorbs damage first)
    int shieldFlash;    // Timer for blue shield flash effect
};

#endif
