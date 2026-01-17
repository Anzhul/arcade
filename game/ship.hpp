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
    virtual void fire(bool button1, bool button2, bool button3, int clock);
    virtual void move(int x, int y, int clock);
    virtual void updateBullets();
    virtual void drawBullets(RGBMatrix *matrix);
    virtual void eraseBullets(RGBMatrix *matrix);
    virtual void updateDistribution(int potentiometer);  // 0-100: 0=max shield, 100=max speed
    virtual void regenerateShield(int clock);

    //Display
    virtual void draw(RGBMatrix *matrix);
    virtual void erase(RGBMatrix *matrix);
    virtual void effects(int x, int y, int clock, RGBMatrix *matrix);

    // Bullet access
    Bullet* getBullets() { return bullets; }

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
    Bullet bullets[MAX_BULLETS];
};

#endif