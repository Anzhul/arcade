#ifndef GAME_HPP
#define GAME_HPP
#include "ship.hpp"
#include "alien.hpp"
#include "led-matrix.h"
using namespace rgb_matrix;

class Game{
public:
    Game();
    ~Game();
    void update(int x, int y, int potentiometer, bool button1, bool button2, bool button3, RGBMatrix *matrix, int clock);
    void setup();
private:
    Ship player;
    //Alien hostile_aliens[];
    //Hostile_ship hostile_ships[];
};

#endif