#ifndef SHIP_HPP
#define SHIP_HPP
/* ship.hpp
 *
 * PLayer ship interface
 *
 * by Anzhu Ling
 * anzhul@umich.edu
 * 2024-12-23
 */

#include <string>
#include <vector>
#include <iostream>
#include "led-matrix.h"
#include "bullet.hpp"
using namespace rgb_matrix;

const int MAX_BULLETS = 20;

class Ship {
public:
    Ship();
    Ship(int health, int speed, int x, int y);
    ~Ship();

    //Getters

    virtual const int get_health();
    virtual const int get_speed();
    virtual const int get_x();
    virtual const int get_y();
    virtual const int get_shield();
    virtual const bool isShieldActive();

    //Setters

    virtual void setHealth(int health);
    virtual void setSpeed(int speed);
    virtual void setX(int x);
    virtual void setY(int y);
    virtual void setShield(int shield);
    virtual void activateShield(bool active);

    //Actions

    virtual void takeDamage(int damage);
    virtual void fire(bool button1, bool button2, bool button3, int clock, RGBMatrix *matrix);
    virtual void updateWeaponMode(int pot2);
    virtual void cycleWeapon();
    virtual void updateLaser(int clock, RGBMatrix *matrix);
    virtual void eraseLaser(RGBMatrix *matrix);
    virtual void move(int x, int y, int clock);
    virtual void updateBullets();
    virtual void drawBullets(RGBMatrix *matrix);
    virtual void eraseBullets(RGBMatrix *matrix);
    virtual void updateDistribution(int potentiometer);  // 0-100: 0=max shield, 100=max speed
    virtual void regenerateShield(int clock);
    virtual void dash(int joystickX, int joystickY, bool button1, int clock, RGBMatrix *matrix);
    virtual void updateDashTrail(int clock, RGBMatrix *matrix);
    virtual void eraseDashTrail(RGBMatrix *matrix);
    virtual void activateShieldBubble(bool button3, int clock, RGBMatrix *matrix);
    virtual void updateShieldBubble(int clock, RGBMatrix *matrix);
    virtual void drawShieldBubble(RGBMatrix *matrix);
    virtual void eraseShieldBubble(RGBMatrix *matrix);

    //Display
    virtual void draw(RGBMatrix *matrix);
    virtual void erase(RGBMatrix *matrix);
    virtual void effects(int x, int y, int clock, RGBMatrix *matrix);

    // Bullet access
    Bullet* getBullets() { return bullets; }
    int getLaserActive() const { return laserActive; }
    int getLastLaserX() const { return lastLaserX; }
    int getLastLaserY() const { return lastLaserY; }

private:
    int health;
    int speed;            // Speed multiplier (10 = 1.0x, 30 = 3.0x)
    int x;
    int y;
    int moveAccumX;       // Accumulated fractional movement
    int moveAccumY;
    int shield;
    int maxShield;
    bool shieldActive;
    int cooldown1;
    int cooldown2;
    int cooldown3;
    int fire_rate;
    int lastShieldRegen;  // Clock time of last shield regen
    int dashCooldown;     // Clock time of last dash
    int dashTrailX[5];    // Trail ship positions (up to 5 ghost ships)
    int dashTrailY[5];
    int dashTrailCount;   // Number of trail positions
    int dashTrailStart;   // Clock time when trail started
    void drawShipAt(int posX, int posY, float opacity, RGBMatrix *matrix);
    void eraseShipAt(int posX, int posY, RGBMatrix *matrix);
    Bullet bullets[MAX_BULLETS];

    // Weapon mode (0=normal, 1=scatter, 2=rocket, 3=laser)
    int weaponMode;
    int laserCharging;    // 0 = not charging, >0 = charging tick count
    int laserActive;      // 0 = inactive, >0 = active tick count
    int laserStartTick;   // Clock tick when laser started
    int lastLaserY;       // Y position where laser was drawn (for erase)
    int lastLaserX;       // X position where laser was drawn (for erase)

    // Shield bubble
    int shieldBubbleActive;    // 0 = inactive, >0 = active tick count
    int shieldBubbleStart;     // Clock tick when bubble started
    int shieldBubbleCooldown;  // Clock tick of last bubble use
    int lastBubbleX;           // Position where bubble was drawn (for erase)
    int lastBubbleY;

    // Dash animation
    bool dashAnimating;        // True while dash animation is in progress
    int dashStartX;            // Starting X position for dash
    int dashStartY;            // Starting Y position for dash
    int dashTargetX;           // Target X position for dash
    int dashTargetY;           // Target Y position for dash
    int dashAnimStart;         // Clock tick when animation started
};

#endif