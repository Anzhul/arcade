
#include "ship.hpp"


Ship::Ship() {
    health = 100;
    speed = 1;
    x = 0;
    y = 0;
}

Ship::Ship(int health, int speed, int x, int y) : health(health), speed(speed), x(x), y(y) {}

Ship::~Ship() {}    

const int Ship::get_health() {
    return health;
}

const int Ship::get_speed() {
    return speed;
}

const int Ship::get_x() {
    return x;
}

const int Ship::get_y() {
    return y;
}

void Ship::setHealth(int health) {
    this->health = health;
}

void Ship::setSpeed(int speed) {
    this->speed = speed;
}

void Ship::setX(int x) {
    this->x = x;
}


void Ship::setY(int y) {
    this->y = y;
}


void Ship::takeDamage(int damage) {
    health -= damage;
}

void Ship::fire() {
}

void Ship::move(int x, int y) {
    this->x = x;
    this->y = y;
}

void Ship::draw(RGBMatrix *matrix) {
    Color color(200, 200, 200);
    matrix->SetPixel(x, y, color.r, color.g, color.b);
    
}

void Ship::erase(RGBMatrix *matrix) {
    Color color(0, 0, 0);
    matrix->SetPixel(x, y, color.r, color.g, color.b);
}