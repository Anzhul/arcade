#ifndef ELITE_ALIEN_HPP
#define ELITE_ALIEN_HPP

#include "alien.hpp"

class EliteAlien : public Alien {
public:
    EliteAlien();
    EliteAlien(int x, int y);
    ~EliteAlien();
    
    void draw(RGBMatrix *matrix) override;
    void erase(RGBMatrix *matrix) override;
    void move(int dx, int dy) override;
    void takeDamage(int damage) override;
    bool shouldShoot();
    void resetShotCooldown();
    int getShield() const;
    void setBehavior(int mode) { behaviorMode = mode; }
    int getBehavior() const { return behaviorMode; }
    bool isDashing() const { return dashing; }

private:
    int horizontalDirection;
    int movePattern;
    int moveCounter;
    int shotCooldown;
    int shield;         // Shield HP (absorbs damage first)
    int shieldFlash;    // Timer for blue shield flash effect
    // Behavior: 0=default (anchor, shoot, then dash), 1=straight, 2=dodging
    int behaviorMode;
    // Default behavior state
    int anchorY;        // Y to anchor at
    bool anchored;      // Whether we've reached anchor point
    int shotsFired;     // Shots fired while anchored
    int shotsBeforeDash; // How many to fire before dashing (5-8)
    bool charging;      // Charging before dash
    int chargeTimer;    // Frames of charge animation
    bool dashing;       // Currently dashing downward
};

#endif
