
#include "ship.hpp"
#include <tuple>
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
    fire_rate = 5;
    lastShieldRegen = 0;
    dashCooldown = 0;
    dashTrailCount = 0;
    dashTrailStart = 0;
    weaponMode = 0;
    laserCharging = 0;
    laserActive = 0;
    laserStartTick = 0;
    lastLaserY = 0;
    lastLaserX = 0;
    shieldBubbleActive = 0;
    shieldBubbleStart = 0;
    shieldBubbleCooldown = 0;
    lastBubbleX = 0;
    lastBubbleY = 0;
    dashAnimating = false;
    dashStartX = 0;
    dashStartY = 0;
    dashTargetX = 0;
    dashTargetY = 0;
    dashAnimStart = 0;
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

void Ship::fire(bool button1, bool button2, bool button3, int clock, RGBMatrix *matrix)
{
    if (weaponMode == 0)
    {
        // Mode 0: Normal single bullet (pot2 0-32)
        // Faster fire rate: fire_rate ranges 20-3, divided by 3 for even faster shooting
        int mode0_rate = fire_rate / 3;
        if (mode0_rate < 1) mode0_rate = 1;
        if (button2 && (clock - cooldown2) >= mode0_rate)
        {
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (!bullets[i].isActive())
                {
                    bullets[i] = Bullet(x, y - 2, 0, -2, 255, 150, 50);
                    cooldown2 = clock;
                    break;
                }
            }
        }
    }
    else if (weaponMode == 1)
    {
        // Mode 1: Scatter shot (8 bullets in spread pattern)
        // Use fire_rate * 3 for slower rate with multiple bullets
        int mode1_rate = fire_rate * 3;
        if (button2 && (clock - cooldown2) >= mode1_rate)
        {
            int bulletsCreated = 0;
            // 8 directions: up, up-left, up-right, left, right, down-left, down-right, slight angles
            int directions[8][2] = {
                {0, -2},   // Straight up (fast)
                {-1, -2},  // Up-left
                {1, -2},   // Up-right
                {-2, -1},  // Left-up
                {2, -1},   // Right-up
                {-1, -1},  // Diagonal left
                {1, -1},   // Diagonal right
                {0, -1}    // Straight up (slower)
            };
            
            for (int i = 0; i < MAX_BULLETS && bulletsCreated < 8; i++)
            {
                if (!bullets[i].isActive())
                {
                    int dx = directions[bulletsCreated][0];
                    int dy = directions[bulletsCreated][1];
                    bullets[i] = Bullet(x, y - 2, dx, dy, 255, 200, 0);  // Orange scatter
                    bullets[i].setScatter(true);  // Mark as scatter shot
                    bulletsCreated++;
                }
            }
            cooldown2 = clock;
        }
    }
    else if (weaponMode == 2)
    {
        // Mode 2: Rocket launcher
        // Use fire_rate * 5 for much slower rate (powerful weapon)
        int mode2_rate = fire_rate * 5;
        if (button2 && (clock - cooldown2) >= mode2_rate)
        {
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (!bullets[i].isActive())
                {
                    bullets[i] = Bullet(x, y - 3, 0, -3, 255, 50, 0);  // Red rocket, faster
                    bullets[i].setLarge(true);   // Rockets are large
                    bullets[i].setRocket(true);  // Mark as rocket
                    cooldown2 = clock;
                    break;
                }
            }
        }
    }
    else if (weaponMode == 3)
    {
        // Mode 3: Charging laser (pot2 75-99)
        // Charge time based on fire_rate: slower at pot=0, faster at pot=100
        int chargeTime = (fire_rate * 7) / 2;  // Scales with fire rate (70-10.5 ticks)

        if (button2)
        {
            if (laserActive == 0)
            {
                // Charging phase - show flashing dot
                laserCharging++;

                // Erase previous charging dot position if ship moved
                if (lastLaserX != x || lastLaserY != y)
                {
                    matrix->SetPixel(lastLaserY - 4, lastLaserX, 18, 10, 14);
                }
                lastLaserX = x;
                lastLaserY = y;

                // Flash the charging dot (on every other 5 ticks)
                if ((laserCharging / 5) % 2 == 0)
                {
                    matrix->SetPixel(y - 4, x, 50, 200, 255);  // Bright cyan
                }
                else
                {
                    matrix->SetPixel(y - 4, x, 18, 10, 14);    // Off (background)
                }

                if (laserCharging >= chargeTime)
                {
                    // Fire laser
                    laserActive = 30;  // Laser lasts 30 ticks
                    laserStartTick = clock;
                    laserCharging = 0;
                }
            }

            // If laser is active, update its position to follow ship
            if (laserActive > 0)
            {
                // Erase previous laser position
                if (lastLaserX != x || lastLaserY != y)
                {
                    for (int ly = lastLaserY - 4; ly >= 0; ly--)
                    {
                        matrix->SetPixel(ly, lastLaserX, 18, 10, 14);
                    }
                }

                // Draw laser at current position (starts 2 pixels in front of ship)
                lastLaserX = x;
                lastLaserY = y;
                for (int ly = y - 4; ly >= 0; ly--)
                {
                    matrix->SetPixel(ly, x, 50, 200, 255);
                }
            }
        }
        else
        {
            // Button released, reset charging and deactivate laser
            if (laserCharging > 0)
            {
                // Erase charging dot
                matrix->SetPixel(lastLaserY - 4, lastLaserX, 18, 10, 14);
            }
            laserCharging = 0;
            if (laserActive > 0)
            {
                eraseLaser(matrix);
                laserActive = 0;
            }
        }
    }
}

void Ship::updateWeaponMode(int pot2)
{
    if (pot2 <= 20)
    {
        weaponMode = 0;  // Normal
    }
    else if (pot2 <= 45)
    {
        weaponMode = 1;  // Scatter shot (8-way)
    }
    else if (pot2 <= 70)
    {
        weaponMode = 2;  // Rocket
    }
    else
    {
        weaponMode = 3;  // Laser
    }
}

void Ship::updateLaser(int clock, RGBMatrix *matrix)
{
    if (laserActive > 0 && (clock - laserStartTick) >= 30)
    {
        // Laser duration expired, erase it
        eraseLaser(matrix);
        laserActive = 0;
    }
}

void Ship::eraseLaser(RGBMatrix *matrix)
{
    for (int ly = lastLaserY - 4; ly >= 0; ly--)
    {
        matrix->SetPixel(ly, lastLaserX, 18, 10, 14);  // Background color
    }
}

void Ship::updateDistribution(int potentiometer)
{
    // potentiometer 0-100: 0 = max shield (100), slow fire rate
    //                    100 = no shield (0), fast fire rate

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

    // Speed is constant at 1.0x
    speed = 10;

    // Calculate fire_rate: 20 at pot=0 (slow), 3 at pot=100 (fast)
    // Linear interpolation: 20 - (potentiometer * 17) / 100
    fire_rate = 20 - (potentiometer * 17) / 100;
}

void Ship::regenerateShield(int clock)
{
    // Regenerate 10 shield per interval
    // More shield capacity = faster regen
    // 10 bars: 0.25s (25 ticks), 5 bars: 0.5s (50 ticks), 1 bar: 2.5s (250 ticks)
    const int REGEN_AMOUNT = 10;
    int shieldBars = maxShield / 10;  // Number of shield bars (0-10)
    if (shieldBars == 0) return;

    int regenInterval = 250 / shieldBars;  // Inverse: more bars = faster

    if (shield < maxShield && (clock - lastShieldRegen) >= regenInterval)
    {
        shield += REGEN_AMOUNT;
        if (shield > maxShield)
        {
            shield = maxShield;
        }
        lastShieldRegen = clock;
    }
}

void Ship::dash(int joystickX, int joystickY, bool button1, int clock, RGBMatrix *matrix)
{
    const int DASH_COOLDOWN = 20;  // 0.5 second at 100fps
    const int DASH_DISTANCE = 10;
    const int DASH_DURATION = 3;   // 3 ticks for animation (faster)
    const int deadzone = 5;

    // If dash animation is in progress, update position
    if (dashAnimating)
    {
        int elapsed = clock - dashAnimStart;

        // Store current position as trail before moving
        if (dashTrailCount < 5)
        {
            dashTrailX[dashTrailCount] = x;
            dashTrailY[dashTrailCount] = y;
            dashTrailCount++;
        }

        if (elapsed >= DASH_DURATION)
        {
            // Animation complete, snap to target
            x = dashTargetX;
            y = dashTargetY;
            dashAnimating = false;
            // Reset trail start time so it fades after animation completes
            dashTrailStart = clock;
        }
        else
        {
            // Interpolate position over 3 ticks
            int progress = elapsed + 1;  // 1 to 3
            x = dashStartX + ((dashTargetX - dashStartX) * progress) / DASH_DURATION;
            y = dashStartY + ((dashTargetY - dashStartY) * progress) / DASH_DURATION;
        }
        return;
    }

    // Don't start new dash if on cooldown or button not pressed
    if (!button1 || (clock - dashCooldown) < DASH_COOLDOWN)
    {
        return;
    }

    // Check if joystick is in a direction
    bool hasDirection = (joystickX > deadzone || joystickX < -deadzone ||
                         joystickY > deadzone || joystickY < -deadzone);

    if (!hasDirection)
    {
        return;  // No direction, stay stationary
    }

    // Store starting position
    dashStartX = x;
    dashStartY = y;
    dashTargetX = x;
    dashTargetY = y;

    // Check if movement is pure X or Y (not diagonal)
    bool movingX = (joystickX > deadzone || joystickX < -deadzone);
    bool movingY = (joystickY > deadzone || joystickY < -deadzone);
    int distance = (movingX && movingY) ? DASH_DISTANCE : DASH_DISTANCE + 5;  // 15 for pure, 10 for diagonal

    // Calculate target position
    if (joystickY > deadzone)
    {
        dashTargetY = y + distance;
        if (dashTargetY > 64) dashTargetY = 64;
    }
    else if (joystickY < -deadzone)
    {
        dashTargetY = y - distance;
        if (dashTargetY < 0) dashTargetY = 0;
    }

    if (joystickX > deadzone)
    {
        dashTargetX = x - distance;
    }
    else if (joystickX < -deadzone)
    {
        dashTargetX = x + distance;
    }

    // Initialize trail with starting position
    dashTrailCount = 1;
    dashTrailX[0] = dashStartX;
    dashTrailY[0] = dashStartY;
    dashTrailStart = clock;

    // Start animation
    dashAnimating = true;
    dashAnimStart = clock;
    dashCooldown = clock;
}

void Ship::updateDashTrail(int clock, RGBMatrix *matrix)
{
    const int TRAIL_DURATION = 5;  // Trail lingers a bit after dash completes

    // Draw trail (during animation or lingering after)
    if (dashTrailCount > 0)
    {
        // Draw all trail positions with decreasing opacity (oldest = most faded)
        for (int i = 0; i < dashTrailCount; i++)
        {
            // Oldest trail (i=0) is most faded, newest is brightest
            float opacity = 0.15f + (0.15f * i / dashTrailCount);
            drawShipAt(dashTrailX[i], dashTrailY[i], opacity, matrix);
        }
    }

    // Only erase trail after animation is done and duration has passed
    if (!dashAnimating && dashTrailCount > 0 && (clock - dashTrailStart) >= TRAIL_DURATION)
    {
        // Erase trail
        eraseDashTrail(matrix);
        dashTrailCount = 0;
    }
}

void Ship::eraseDashTrail(RGBMatrix *matrix)
{
    for (int i = 0; i < dashTrailCount; i++)
    {
        eraseShipAt(dashTrailX[i], dashTrailY[i], matrix);
    }
}

void Ship::drawShipAt(int posX, int posY, float opacity, RGBMatrix *matrix)
{
    // Background color for blending
    const int bgR = 18, bgG = 10, bgB = 14;

    // Helper lambda to blend color with background
    auto blend = [&](int r, int g, int b) -> std::tuple<int, int, int> {
        return {
            (int)(bgR + (r - bgR) * opacity),
            (int)(bgG + (g - bgG) * opacity),
            (int)(bgB + (b - bgB) * opacity)
        };
    };

    auto [r1, g1, b1] = blend(99, 154, 205);   // Blue cockpit
    auto [r2, g2, b2] = blend(200, 200, 185);  // Body
    auto [r3, g3, b3] = blend(132, 132, 132);  // Gray
    auto [r4, g4, b4] = blend(238, 141, 105);  // Orange
    auto [r5, g5, b5] = blend(122, 109, 103);  // Brown
    auto [r6, g6, b6] = blend(83, 75, 71);     // Dark
    auto [r7, g7, b7] = blend(200, 200, 200);  // Light

    matrix->SetPixel(posY, posX, r1, g1, b1);
    matrix->SetPixel(posY, posX + 1, r2, g2, b2);
    matrix->SetPixel(posY, posX + 2, r2, g2, b2);
    matrix->SetPixel(posY, posX - 1, r2, g2, b2);
    matrix->SetPixel(posY, posX - 2, r2, g2, b2);
    matrix->SetPixel(posY + 1, posX, r2, g2, b2);
    matrix->SetPixel(posY - 1, posX, r2, g2, b2);
    matrix->SetPixel(posY - 1, posX + 1, r2, g2, b2);
    matrix->SetPixel(posY - 1, posX - 1, r2, g2, b2);
    matrix->SetPixel(posY - 2, posX, r1, g1, b1);
    matrix->SetPixel(posY + 2, posX, r7, g7, b7);
    matrix->SetPixel(posY + 1, posX + 1, r3, g3, b3);
    matrix->SetPixel(posY + 1, posX - 1, r3, g3, b3);
    matrix->SetPixel(posY + 1, posX + 2, r4, g4, b4);
    matrix->SetPixel(posY + 1, posX - 2, r4, g4, b4);
    matrix->SetPixel(posY + 1, posX + 3, r2, g2, b2);
    matrix->SetPixel(posY + 1, posX - 3, r2, g2, b2);
    matrix->SetPixel(posY + 2, posX + 1, r3, g3, b3);
    matrix->SetPixel(posY + 2, posX - 1, r3, g3, b3);
    matrix->SetPixel(posY + 2, posX, r5, g5, b5);
    matrix->SetPixel(posY + 3, posX + 1, r6, g6, b6);
    matrix->SetPixel(posY + 3, posX - 1, r6, g6, b6);
}

void Ship::eraseShipAt(int posX, int posY, RGBMatrix *matrix)
{
    const int r = 18, g = 10, b = 14;  // Background color
    matrix->SetPixel(posY, posX, r, g, b);
    matrix->SetPixel(posY, posX + 1, r, g, b);
    matrix->SetPixel(posY, posX + 2, r, g, b);
    matrix->SetPixel(posY, posX - 1, r, g, b);
    matrix->SetPixel(posY, posX - 2, r, g, b);
    matrix->SetPixel(posY + 1, posX, r, g, b);
    matrix->SetPixel(posY - 1, posX, r, g, b);
    matrix->SetPixel(posY - 1, posX + 1, r, g, b);
    matrix->SetPixel(posY - 1, posX - 1, r, g, b);
    matrix->SetPixel(posY - 2, posX, r, g, b);
    matrix->SetPixel(posY + 2, posX, r, g, b);
    matrix->SetPixel(posY + 1, posX + 1, r, g, b);
    matrix->SetPixel(posY + 1, posX - 1, r, g, b);
    matrix->SetPixel(posY + 1, posX + 2, r, g, b);
    matrix->SetPixel(posY + 1, posX - 2, r, g, b);
    matrix->SetPixel(posY + 1, posX + 3, r, g, b);
    matrix->SetPixel(posY + 1, posX - 3, r, g, b);
    matrix->SetPixel(posY + 2, posX + 1, r, g, b);
    matrix->SetPixel(posY + 2, posX - 1, r, g, b);
    matrix->SetPixel(posY + 2, posX, r, g, b);
    matrix->SetPixel(posY + 3, posX + 1, r, g, b);
    matrix->SetPixel(posY + 3, posX - 1, r, g, b);
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

void Ship::activateShieldBubble(bool button3, int clock, RGBMatrix *matrix)
{
    const int BUBBLE_COOLDOWN = 45;
    const int BUBBLE_DURATION = 30;  // How long the bubble lasts

    if (button3 && shieldBubbleActive == 0 && (clock - shieldBubbleCooldown) >= BUBBLE_COOLDOWN)
    {
        // Activate shield bubble
        shieldBubbleActive = BUBBLE_DURATION;
        shieldBubbleStart = clock;
        shieldBubbleCooldown = clock;
        lastBubbleX = x;
        lastBubbleY = y;
        drawShieldBubble(matrix);
    }
}

void Ship::updateShieldBubble(int clock, RGBMatrix *matrix)
{
    if (shieldBubbleActive > 0)
    {
        // Erase previous bubble position if ship moved
        if (lastBubbleX != x || lastBubbleY != y)
        {
            eraseShieldBubble(matrix);
        }

        // Update position
        lastBubbleX = x;
        lastBubbleY = y;

        // Blink every 3 ticks
        if (clock % 3 != 0)
        {
            drawShieldBubble(matrix);
        }
        else
        {
            eraseShieldBubble(matrix);
        }

        // Check if bubble duration expired
        if ((clock - shieldBubbleStart) >= shieldBubbleActive)
        {
            eraseShieldBubble(matrix);
            shieldBubbleActive = 0;
        }
    }
}

void Ship::drawShieldBubble(RGBMatrix *matrix)
{
    // Draw a circular shield around the ship (radius ~5 pixels)
    // Using a simple circle pattern
    int cx = x;
    int cy = y;

    // Shield color - dimmer cyan/blue glow (lower opacity)
    int r = 30, g = 80, b = 140;

    // Top and bottom
    matrix->SetPixel(cy - 5, cx, r, g, b);
    matrix->SetPixel(cy - 5, cx - 1, r, g, b);
    matrix->SetPixel(cy - 5, cx + 1, r, g, b);
    matrix->SetPixel(cy + 5, cx, r, g, b);
    matrix->SetPixel(cy + 5, cx - 1, r, g, b);
    matrix->SetPixel(cy + 5, cx + 1, r, g, b);

    // Upper corners
    matrix->SetPixel(cy - 4, cx - 3, r, g, b);
    matrix->SetPixel(cy - 4, cx + 3, r, g, b);
    matrix->SetPixel(cy - 3, cx - 4, r, g, b);
    matrix->SetPixel(cy - 3, cx + 4, r, g, b);

    // Lower corners
    matrix->SetPixel(cy + 4, cx - 3, r, g, b);
    matrix->SetPixel(cy + 4, cx + 3, r, g, b);
    matrix->SetPixel(cy + 3, cx - 4, r, g, b);
    matrix->SetPixel(cy + 3, cx + 4, r, g, b);

    // Sides
    matrix->SetPixel(cy - 2, cx - 5, r, g, b);
    matrix->SetPixel(cy - 1, cx - 5, r, g, b);
    matrix->SetPixel(cy, cx - 5, r, g, b);
    matrix->SetPixel(cy + 1, cx - 5, r, g, b);
    matrix->SetPixel(cy + 2, cx - 5, r, g, b);

    matrix->SetPixel(cy - 2, cx + 5, r, g, b);
    matrix->SetPixel(cy - 1, cx + 5, r, g, b);
    matrix->SetPixel(cy, cx + 5, r, g, b);
    matrix->SetPixel(cy + 1, cx + 5, r, g, b);
    matrix->SetPixel(cy + 2, cx + 5, r, g, b);
}

void Ship::eraseShieldBubble(RGBMatrix *matrix)
{
    int cx = lastBubbleX;
    int cy = lastBubbleY;

    // Background color
    int r = 18, g = 10, b = 14;

    // Top and bottom
    matrix->SetPixel(cy - 5, cx, r, g, b);
    matrix->SetPixel(cy - 5, cx - 1, r, g, b);
    matrix->SetPixel(cy - 5, cx + 1, r, g, b);
    matrix->SetPixel(cy + 5, cx, r, g, b);
    matrix->SetPixel(cy + 5, cx - 1, r, g, b);
    matrix->SetPixel(cy + 5, cx + 1, r, g, b);

    // Upper corners
    matrix->SetPixel(cy - 4, cx - 3, r, g, b);
    matrix->SetPixel(cy - 4, cx + 3, r, g, b);
    matrix->SetPixel(cy - 3, cx - 4, r, g, b);
    matrix->SetPixel(cy - 3, cx + 4, r, g, b);

    // Lower corners
    matrix->SetPixel(cy + 4, cx - 3, r, g, b);
    matrix->SetPixel(cy + 4, cx + 3, r, g, b);
    matrix->SetPixel(cy + 3, cx - 4, r, g, b);
    matrix->SetPixel(cy + 3, cx + 4, r, g, b);

    // Sides
    matrix->SetPixel(cy - 2, cx - 5, r, g, b);
    matrix->SetPixel(cy - 1, cx - 5, r, g, b);
    matrix->SetPixel(cy, cx - 5, r, g, b);
    matrix->SetPixel(cy + 1, cx - 5, r, g, b);
    matrix->SetPixel(cy + 2, cx - 5, r, g, b);

    matrix->SetPixel(cy - 2, cx + 5, r, g, b);
    matrix->SetPixel(cy - 1, cx + 5, r, g, b);
    matrix->SetPixel(cy, cx + 5, r, g, b);
    matrix->SetPixel(cy + 1, cx + 5, r, g, b);
    matrix->SetPixel(cy + 2, cx + 5, r, g, b);
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
