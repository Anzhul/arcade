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
using namespace rgb_matrix;

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

    //Setters

    virtual void setHealth(int health);
    virtual void setSpeed(int speed);
    virtual void setX(int x);
    virtual void setY(int y);

    //Actions

    virtual void takeDamage(int damage);
    virtual void fire(int button1, int button2, int button3, int clock);
    virtual void move(int x, int y, int clock);

    //Display
    virtual void draw(RGBMatrix *matrix);
    virtual void erase(RGBMatrix *matrix);
    virtual void effects(int x, int y, int clock, RGBMatrix *matrix);

private:
    int health;
    int speed;
    int x;
    int y;
    int cooldown2;
    int cooldown3;
    int fire_rate;
};

#endif