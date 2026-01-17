
#include "ship.hpp"
using namespace std;

Ship::Ship()
{
    health = 100;
    speed = 10;  // 10 = 1.0x speed (using fixed-point with 10 as base)
    x = 64 / 2;
    y = 60;
    moveAccumX = 0;
    moveAccumY = 0;
    shield = 50;
    maxShield = 50;
    shieldActive = true;
    cooldown1 = 0;
    cooldown2 = 0;
    cooldown3 = 0;
    fire_rate = 10;
    lastShieldRegen = 0;
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

const int Ship::get_shield()
{
    return shield;
}

const bool Ship::isShieldActive()
{
    return shieldActive;
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

void Ship::setShield(int shield)
{
    this->shield = shield;
}

void Ship::activateShield(bool active)
{
    this->shieldActive = active;
}

void Ship::takeDamage(int damage)
{
    if (shieldActive && shield > 0)
    {
        shield -= damage;
        if (shield < 0)
        {
            health += shield;  // Apply overflow damage to health
            shield = 0;
            shieldActive = false;
        }
    }
    else
    {
        health -= damage;
    }
}

void Ship::fire(bool button1, bool button2, bool button3, int clock)
{
    // Fire bullet when button2 is pressed and cooldown has passed
    if (button2 && (clock - cooldown2) >= fire_rate)
    {
        // Find an inactive bullet slot
        for (int i = 0; i < MAX_BULLETS; i++)
        {
            if (!bullets[i].isActive())
            {
                // Create bullet at ship center, moving up (negative y direction)
                bullets[i] = Bullet(x, y - 2, 0, -2, 255, 50, 50);  // Red bullet
                cooldown2 = clock;
                break;
            }
        }
    }
}

void Ship::updateDistribution(int potentiometer)
{
    // potentiometer 0-100: 0 = max shield (100), min speed (5 = 0.5x)
    //                    100 = no shield (0), max speed (30 = 3.0x)

    // Calculate new max shield (100 at pot=0, 0 at pot=100)
    maxShield = 100 - potentiometer;

    // Cap current shield to max (don't give free shield when switching)
    if (shield > maxShield)
    {
        shield = maxShield;
    }

    // Shield is only active if we have capacity for it
    if (maxShield == 0)
    {
        shieldActive = false;
        shield = 0;
    }
    else
    {
        shieldActive = true;
    }

    // Calculate speed: 10 at pot=0 (1.0x), 30 at pot=100 (3.0x)
    // Linear interpolation: 10 + (potentiometer * 20) / 100
    speed = 10 + (potentiometer * 20) / 100;
}

void Ship::regenerateShield(int clock)
{
    // Regenerate 10 shield per interval
    // Base interval: 0.25s per shield bar (25 ticks per bar at 100fps)
    // 10 bars = 2.5s, 5 bars = 1.25s, 1 bar = 0.25s
    const int REGEN_AMOUNT = 10;
    int shieldBars = maxShield / 10;  // Number of shield bars (0-10)
    int regenInterval = shieldBars * 25;  // 0.25s per bar

    if (maxShield > 0 && shield < maxShield && regenInterval > 0 &&
        (clock - lastShieldRegen) >= regenInterval)
    {
        shield += REGEN_AMOUNT;
        if (shield > maxShield)
        {
            shield = maxShield;
        }
        lastShieldRegen = clock;
    }
}

void Ship::updateBullets()
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        bullets[i].update();
    }
}

void Ship::drawBullets(RGBMatrix *matrix)
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        bullets[i].draw(matrix);
    }
}

void Ship::eraseBullets(RGBMatrix *matrix)
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        bullets[i].erase(matrix);
    }
}

void Ship::move(int x, int y, int clock)
{
    // x, y are -100 to 100 (0 = center)
    // speed is fixed-point: 10 = 1.0x, 5 = 0.5x, 30 = 3.0x
    const int deadzone = 10;
    const int BASE = 10;  // Fixed-point base

    // Accumulate movement based on joystick input
    if (y > deadzone && this->y < 64)
    {
        moveAccumY += speed;
    }
    else if (y < -deadzone && this->y > 0)
    {
        moveAccumY -= speed;
    }

    if (x > deadzone)
    {
        moveAccumX -= speed;
    }
    else if (x < -deadzone)
    {
        moveAccumX += speed;
    }

    // Apply accumulated movement when it reaches a full pixel
    while (moveAccumY >= BASE)
    {
        if (this->y < 64) this->y++;
        moveAccumY -= BASE;
    }
    while (moveAccumY <= -BASE)
    {
        if (this->y > 0) this->y--;
        moveAccumY += BASE;
    }
    while (moveAccumX >= BASE)
    {
        this->x++;
        moveAccumX -= BASE;
    }
    while (moveAccumX <= -BASE)
    {
        this->x--;
        moveAccumX += BASE;
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
    // Erase thruster effect pixels
    matrix->SetPixel(y + 4, x + 1, color.r, color.g, color.b);
    matrix->SetPixel(y + 4, x - 1, color.r, color.g, color.b);
    matrix->SetPixel(y - 1, x + 3, color.r, color.g, color.b);
    matrix->SetPixel(y - 1, x - 3, color.r, color.g, color.b);
}

void Ship::effects(int x, int y, int clock, RGBMatrix *matrix)
{
    // x, y are -100 to 100 (0 = center)
    // Show thrust effects based on joystick direction
    if (clock % 2 == 0 && y > 20)
    {
        // Moving down - show reverse thrusters
        matrix->SetPixel(this->y - 1, this->x + 3, 68, 70, 90);
        matrix->SetPixel(this->y - 1, this->x - 3, 68, 70, 90);
        matrix->SetPixel(this->y + 4, this->x + 1, 18, 10, 14);
        matrix->SetPixel(this->y + 4, this->x - 1, 18, 10, 14);
    }
    else if (clock % 2 == 0 && y < -20)
    {
        // Moving up - show main thrusters
        matrix->SetPixel(this->y + 4, this->x + 1, 179, 234, 255);
        matrix->SetPixel(this->y + 4, this->x - 1, 179, 234, 255);
        matrix->SetPixel(this->y - 1, this->x + 3, 18, 10, 14);
        matrix->SetPixel(this->y - 1, this->x - 3, 18, 10, 14);
    }
    else if (clock % 2 == 0)
    {
        // Idle - dim thrusters
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
    // Wrap around screen edges
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
