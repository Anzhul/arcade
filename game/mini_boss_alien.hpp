#ifndef MINI_BOSS_ALIEN_HPP
#define MINI_BOSS_ALIEN_HPP

#include "alien.hpp"

class MiniBossAlien : public Alien {
public:
    MiniBossAlien();
    MiniBossAlien(int x, int y);
    ~MiniBossAlien();

    void draw(RGBMatrix *matrix) override;
    void move(int dx, int dy) override;
    void takeDamage(int damage) override;
    void erase(RGBMatrix *matrix) override;

    int getShield() const;
    bool shouldShoot();
    void resetShotCooldown();
    void setShotCooldown(int cd);
    bool isEntryComplete() const;
    void updateTurret(int playerX);  // Track turret toward player
    int getTurretX() const;          // Turret offset from center

private:
    int shield;
    int shieldFlash;
    int shotCooldown;
    int moveCounter;
    int horizontalDirection;
    int frameCounter;
    bool entryComplete;
    int turretOffset;  // -3 to +3, turret x offset from center
};

#endif
