#ifndef STAR_HPP
#define STAR_HPP

#include "led-matrix.h"
using namespace rgb_matrix; 

class Star{
public:
    Star();
    Star(int x, int y, int size, int speed);
    ~Star();

    //Getters

    virtual const int get_x();
    virtual const int get_y();

    //Setters

    virtual void setX(int x);
    virtual void setY(int y);

    //Actions

    virtual void move(int x, int y, int clock);

    //Display
    virtual void draw(RGBMatrix *matrix);
    virtual void erase(RGBMatrix *matrix);
    
};


#endif