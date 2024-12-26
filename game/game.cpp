#include "game.hpp"

Game::Game() {

}

Game::~Game() {
    
}   

void Game::setup() {
    player = Ship();
}

void Game::update(int x, int y, int potentiometer, bool button1, bool button2, bool button3, RGBMatrix *matrix, int clock) {
    player.erase(matrix);
    player.move((x-127), (y-127), clock);
    player.effects(x, y, clock, matrix);
    player.draw(matrix);
    player.fire(button1, button2, button3, clock);
    player.takeDamage(1);
}