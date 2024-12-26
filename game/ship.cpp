
#include "ship.hpp"
using namespace std;

Ship::Ship()
{
    health = 100;
    speed = 1;
    x = 64 / 2;
    y = 60;
}

Ship::Ship(int health, int speed, int x, int y) : health(health), speed(speed), x(x), y(y) {}

Ship::~Ship() {}

const int Ship::get_health()
{
    return health;
}

const int Ship::get_speed()
{
    return speed;
}

const int Ship::get_x()
{
    return x;
}

const int Ship::get_y()
{
    return y;
}

void Ship::setHealth(int health)
{
    this->health = health;
}

void Ship::setSpeed(int speed)
{
    this->speed = speed;
}

void Ship::setX(int x)
{
    this->x = x;
}

void Ship::setY(int y)
{
    this->y = y;
}

void Ship::takeDamage(int damage)
{
    health -= damage;
}

void Ship::fire(int button1, int button2, int button3)
{
    if (button1 == 0)
    {
        cout << "pew pew" << endl;
    }
    // cout << "pew pew" << endl;
}

void Ship::move(int x, int y, int clock)
{
    if (clock % 10 == 0 && (y - 127) > 0 && this->y < 64)
    {
        this->y += y / 70;
    }
    else if (clock % 10 == 0 && (y - 127) < 0 && this->y > 0)
    {
        this->y += y / 50;
    }
    if (clock % 10 == 0)
    {
        this->x -= x / 60;
    }

}

void Ship::draw(RGBMatrix *matrix)
{
    matrix->SetPixel(y, x, 99, 154, 205);
    matrix->SetPixel(y, x + 1, 200, 200, 185);
    matrix->SetPixel(y, x + 2, 200, 200, 185);
    matrix->SetPixel(y, x - 1, 200, 200, 185);
    matrix->SetPixel(y, x - 2, 200, 200, 185);
    matrix->SetPixel(y + 1, x, 200, 200, 185);
    matrix->SetPixel(y - 1, x, 200, 200, 185);
    matrix->SetPixel(y - 1, x + 1, 200, 200, 185);
    matrix->SetPixel(y - 1, x - 1, 200, 200, 185);
    matrix->SetPixel(y - 2, x, 99, 154, 205);
    matrix->SetPixel(y + 2, x, 200, 200, 200);
    matrix->SetPixel(y + 1, x + 1, 132, 132, 132);
    matrix->SetPixel(y + 1, x - 1, 132, 132, 132);
    matrix->SetPixel(y + 1, x + 2, 238, 141, 105);
    matrix->SetPixel(y + 1, x - 2, 238, 141, 105);
    matrix->SetPixel(y + 1, x + 3, 200, 200, 185);
    matrix->SetPixel(y + 1, x - 3, 200, 200, 185);
    matrix->SetPixel(y + 2, x + 1, 132, 132, 132);
    matrix->SetPixel(y + 2, x - 1, 132, 132, 132);
    matrix->SetPixel(y + 2, x, 122, 109, 103);
    matrix->SetPixel(y + 3, x + 1, 83, 75, 71);
    matrix->SetPixel(y + 3, x - 1, 83, 75, 71);
}

void Ship::erase(RGBMatrix *matrix)
{
    Color color(18, 10, 14);
    matrix->SetPixel(y, x, color.r, color.g, color.b);
    matrix->SetPixel(y, x + 1, color.r, color.g, color.b);
    matrix->SetPixel(y, x + 2, color.r, color.g, color.b);
    matrix->SetPixel(y, x - 1, color.r, color.g, color.b);
    matrix->SetPixel(y, x - 2, color.r, color.g, color.b);
    matrix->SetPixel(y + 1, x, color.r, color.g, color.b);
    matrix->SetPixel(y - 1, x, color.r, color.g, color.b);
    matrix->SetPixel(y - 1, x + 1, color.r, color.g, color.b);
    matrix->SetPixel(y - 1, x - 1, color.r, color.g, color.b);
    matrix->SetPixel(y - 2, x, color.r, color.g, color.b);
    matrix->SetPixel(y + 2, x, color.r, color.g, color.b);
    matrix->SetPixel(y + 1, x + 1, color.r, color.g, color.b);
    matrix->SetPixel(y + 1, x - 1, color.r, color.g, color.b);
    matrix->SetPixel(y + 1, x + 2, color.r, color.g, color.b);
    matrix->SetPixel(y + 1, x - 2, color.r, color.g, color.b);
    matrix->SetPixel(y + 1, x + 3, color.r, color.g, color.b);
    matrix->SetPixel(y + 1, x - 3, color.r, color.g, color.b);
    matrix->SetPixel(y + 2, x + 1, color.r, color.g, color.b);
    matrix->SetPixel(y + 2, x - 1, color.r, color.g, color.b);
    matrix->SetPixel(y + 2, x, color.r, color.g, color.b);
    matrix->SetPixel(y + 3, x + 1, color.r, color.g, color.b);
    matrix->SetPixel(y + 3, x - 1, color.r, color.g, color.b);
}

void Ship::effects(int x, int y, int clock, RGBMatrix *matrix)
{
    //Reverse the ship
    if (clock % 2 == 0 && (y - 127) / 50 > 0)
    {
        matrix->SetPixel(this->y - 1, this->x + 3, 121, 134, 151);
        matrix->SetPixel(this->y - 1, this->x - 3, 121, 134, 151);
        matrix->SetPixel(this->y + 4, this->x + 1, 18, 10, 14);
        matrix->SetPixel(this->y + 4, this->x - 1, 18, 10, 14);
    }
    else if (clock % 2 == 0 && (y - 127) / 50 < 0)
    {
        matrix->SetPixel(this->y + 4, this->x + 1, 179, 234, 255);
        matrix->SetPixel(this->y + 4, this->x - 1, 179, 234, 255);
        matrix->SetPixel(this->y - 1, this->x + 3, 18, 10, 14);
        matrix->SetPixel(this->y - 1, this->x - 3, 18, 10, 14);
    }
    else if (clock % 2 == 0)
    {
        matrix->SetPixel(this->y - 1, this->x + 3, 18, 10, 14);
        matrix->SetPixel(this->y - 1, this->x - 3, 18, 10, 14);
        matrix->SetPixel(this->y + 4, this->x + 1, 68, 70, 90);
        matrix->SetPixel(this->y + 4, this->x - 1, 68, 70, 90);
    }
    else
    {
        matrix->SetPixel(this->y - 1, this->x + 3, 18, 10, 14);
        matrix->SetPixel(this->y - 1, this->x - 3, 18, 10, 14);
        matrix->SetPixel(this->y + 4, this->x + 1, 18, 10, 14);
        matrix->SetPixel(this->y + 4, this->x - 1, 18, 10, 14);
    }
    if (this->x+3 < 0){
        this->x = 67;
    }
    else if (this->x-3 > 64){
        this->x = -3;
    }
}

class Hostile_ship : public Ship
{
public:
    Hostile_ship();
    Hostile_ship(int health, int speed, int x, int y);
    ~Hostile_ship();

    // Getters

    const int get_health();
    const int get_speed();
    const int get_x();
    const int get_y();

    // Setters

    void setHealth(int health);
    void setSpeed(int speed);
    void setX(int x);
    void setY(int y);

    // Actions

    void takeDamage(int damage);
    void fire();
    void move(int x, int y);

    // Display
    void draw(RGBMatrix *matrix);
    void erase(RGBMatrix *matrix);
};