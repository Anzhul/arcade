#ifndef ALIEN_HPP
#define ALIEN_HPP
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
#include "led-matrix.h"
using namespace rgb_matrix;

class Alien{
public:
    Alien();
    Alien(int health, int speed, int x, int y);
    ~Alien();

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
    virtual void fire();
    virtual void move(int x, int y);

    //Display
    virtual void draw(RGBMatrix *matrix);
    virtual void erase(RGBMatrix *matrix);
private:
    int health;
    int speed;
    int x;
    int y;
};

#endif