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
    bool shouldShoot();  // Check if alien should shoot this frame
    void resetShotCooldown();  // Reset cooldown after shooting
    
private:
    int horizontalDirection;  // -1 for left, 1 for right
    int movePattern;          // Random movement pattern (0-2)
    int moveCounter;          // Counter for movement changes
    int shotCooldown;         // Frames until next shot
};

#endif
