#ifndef ROCKET_BOSS_HPP
#define ROCKET_BOSS_HPP

#include "alien.hpp"

class RocketBoss : public Alien {
public:
    RocketBoss();
    RocketBoss(int x, int y);
    ~RocketBoss();

    void draw(RGBMatrix *matrix) override;
    void erase(RGBMatrix *matrix) override;
    void move(int dx, int dy) override;
    void takeDamage(int damage) override;

    int getShield() const;
    bool shouldShoot();
    void resetShotCooldown();
    bool isEntryComplete() const;
    int getAttackMode() const { return attackMode; }  // 0 = rockets, 1 = scatter
    int getPhase();    // 1 = single rockets, 2 = dual rocket strings
    int getRocketBurst() const { return rocketBurst; }

private:
    int shield;
    int shieldFlash;
    int shotCooldown;
    int moveCounter;
    int horizontalDirection;
    int frameCounter;
    bool entryComplete;
    int attackMode;       // 0 = homing rockets, 1 = scatter
    int rocketBurst;      // Rockets left in current burst
};

#endif
