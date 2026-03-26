#ifndef BOSS_ALIEN_HPP
#define BOSS_ALIEN_HPP

#include "alien.hpp"

class BossAlien : public Alien {
public:
    BossAlien();
    BossAlien(int x, int y);
    ~BossAlien();

    void draw(RGBMatrix *matrix) override;
    void move(int dx, int dy) override;
    void takeDamage(int damage) override;
    void erase(RGBMatrix *matrix) override;

    int getPhase() const;
    int getShield() const;
    bool shouldShoot();
    void resetShotCooldown();
    bool shouldSpawnMinion();
    void resetMinionCooldown();
    bool isEntryComplete() const;
    bool wantsExplosion() const;  // Boss requests an explosion effect
    int getRedIntensity() const;  // How bright the red warning lights are (0-255)

private:
    int phase;
    int maxHealth;
    int shield;
    int shieldFlash;
    int shotCooldown;
    int moveCounter;
    int horizontalDirection;
    int phaseTransitionTimer;
    int minionSpawnCooldown;
    int frameCounter;
    bool entryComplete;
    int shieldBreakFlash;
    int redLightIntensity;    // Gradually increases after phase 1
    bool explosionRequest;    // Set true when boss wants game to spawn explosion
    // Weak points (exposed after shield down) - relative to boss center
    // Left: (x-6, y+2), Center: (x, y+5), Right: (x+6, y+2)
    static const int WEAK_POINT_RADIUS = 2;
};

#endif
