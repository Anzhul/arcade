#ifndef BEAM_ALIEN_HPP
#define BEAM_ALIEN_HPP

#include "alien.hpp"

class BeamAlien : public Alien {
public:
    BeamAlien();
    BeamAlien(int x, int y);
    ~BeamAlien();

    void draw(RGBMatrix *matrix) override;
    void erase(RGBMatrix *matrix) override;
    void move(int dx, int dy) override;

    bool isCharging() const { return chargeTimer > 0; }
    bool isFiring() const { return beamTimer > 0; }
    int getChargeTimer() const { return chargeTimer; }
    int getBeamTimer() const { return beamTimer; }
    int getBeamX() const;  // X position of beam (can differ from body)
    void startCharge();
    void tickBeam();       // Called each frame to advance charge/beam state

private:
    int horizontalDirection;
    int moveCounter;
    int beamCooldown;      // Frames until next beam
    int chargeTimer;       // Charging countdown (20 frames)
    int beamTimer;         // Active beam duration (40 frames)
    int anchorY;
    bool anchored;
    int beamTargetX;       // X where beam will fire
};

#endif
