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

enum AlienType {
    BASIC,      // Green, 30 HP
    FAST,       // Yellow, 20 HP, moves faster
    TANK,       // Red, 60 HP, slower
    ELITE       // Purple, 40 HP, medium speed
};

class Alien{
public:
    Alien();
    Alien(int health, int speed, int x, int y, AlienType type = BASIC);
    virtual ~Alien();

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
    virtual AlienType getType() const;

        //Actions

    virtual void takeDamage(int damage);
    virtual void fire();
    virtual void move(int x, int y);

    //Display
    virtual void draw(RGBMatrix *matrix);
    virtual void erase(RGBMatrix *matrix);
    virtual void updateDeathAnimation();
    virtual bool isDead() const;
private:
    int health;
    int speed;
    int x;
    int y;
    int deathTimer;  // Timer for death animation
    bool dying;      // Whether enemy is in death animation
    AlienType type;  // Type of alien
};

#endif