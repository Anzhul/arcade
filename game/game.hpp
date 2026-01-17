#ifndef GAME_HPP
#define GAME_HPP
#include "ship.hpp"
#include "alien.hpp"
#include "led-matrix.h"
#include "input.hpp"
using namespace rgb_matrix;

class Game{
public:
    Game();
    ~Game();
    void update(const InputState& input, RGBMatrix *matrix, int clock);
    void setup();
private:
    void drawHUD(RGBMatrix *matrix, int potentiometer);
    Ship player;
    //Alien hostile_aliens[];
    //Hostile_ship hostile_ships[];
};

#endif