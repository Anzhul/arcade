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
    void updateDeathAnimation() override;
    bool isDead() const override;

    int getPhase() const;
    int getShield() const;
    bool shouldShoot();
    void resetShotCooldown();
    int getAttackPattern() const;  // 0 = scatter, 1 = beam
    int getBeamTimer() const;
    int getBeamChargeTimer() const;
    bool shouldSpawnMinion();
    void resetMinionCooldown();
    bool isEntryComplete() const;
    bool wantsExplosion() const;
    int getRedIntensity() const;
    void setPhase(int p);

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
    int redLightIntensity;
    bool explosionRequest;
    int bossDeathTimer;
    bool bossDying;
    int attackPattern;        // 0 = scatter, 1 = beam (alternates in phase 4)
    int beamTimer;            // How long the beam lasts
    int beamChargeTimer;      // Charging before beam fires
    // Weak points (exposed after shield down) - relative to boss center
    // Left: (x-6, y+2), Center: (x, y+5), Right: (x+6, y+2)
    static const int WEAK_POINT_RADIUS = 2;
};

#endif
