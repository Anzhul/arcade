#ifndef ANCHORED_ELITE_HPP
#define ANCHORED_ELITE_HPP

#include "alien.hpp"

class AnchoredElite : public Alien {
public:
    AnchoredElite();
    AnchoredElite(int x, int y);
    ~AnchoredElite();

    void draw(RGBMatrix *matrix) override;
    void erase(RGBMatrix *matrix) override;
    void move(int dx, int dy) override;
    void takeDamage(int damage) override;
    bool shouldShoot();
    void resetShotCooldown();
    int getShield() const;

    void setBeamMode(bool b) { hasBeam = b; }
    bool isBeamMode() const { return hasBeam; }
    bool isCharging() const { return chargeTimer > 0; }
    bool isFiring() const { return beamTimer > 0; }
    int getBeamX() const { return beamTargetX; }
    void tickBeam();

private:
    int horizontalDirection;
    int movePattern;
    int moveCounter;
    int shotCooldown;
    int shield;
    int shieldFlash;
    int anchorY;
    bool anchored;
    // Beam mode
    bool hasBeam;
    int chargeTimer;
    int beamTimer;
    int beamCooldown;
    int beamTargetX;
};

#endif
