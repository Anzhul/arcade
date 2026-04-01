#include "alien.hpp"

Alien::Alien() {
    health = 10;
    speed = 2;
    x = 32;
    y = 5;
    deathTimer = 0;
    dying = false;
    type = BASIC;
    hitFlashTimer = 0;
}

Alien::Alien(int health_in, int speed_in, int x_in, int y_in, AlienType type_in) {
    health = health_in;
    speed = speed_in;
    x = x_in;
    y = y_in;
    deathTimer = 0;
    dying = false;
    type = type_in;
    hitFlashTimer = 0;
}

Alien::~Alien() {

}

// Getters
const int Alien::get_health() {
    return health;
}

const int Alien::get_speed() {
    return speed;
}

const int Alien::get_x() {
    return x;
}

const int Alien::get_y() {
    return y;
}

// Setters
void Alien::setHealth(int health_in) {
    health = health_in;
}

void Alien::setSpeed(int speed_in) {
    speed = speed_in;
}

void Alien::setX(int x_in) {
    x = x_in;
}

void Alien::setY(int y_in) {
    y = y_in;
}

AlienType Alien::getType() const {
    return type;
}

// Actions
void Alien::takeDamage(int damage) {
    if (dying) return;  // Already dying

    health -= damage;
    if (hitFlashTimer <= 0) hitFlashTimer = 6;  // Only flash if previous flash finished
    if (health <= 0) {
        health = 0;
        dying = true;
        deathTimer = 0;
    }
}

int Alien::getHitFlash() const {
    return hitFlashTimer;
}

void Alien::tickHitFlash() {
    if (hitFlashTimer > 0) hitFlashTimer--;
}

void Alien::fire() {
    // TODO: Implement enemy firing
}

void Alien::move(int dx, int dy) {
    x += dx;
    y += dy;
}

// Display
void Alien::draw(RGBMatrix *matrix) {
    if (health < 0) return;  // Completely dead, don't draw
    
    // Bounds check - allow partial drawing at bottom edge
    if (x < 0 || x >= 64 || y < 0 || y > 70) return;
    
    if (dying) {
        int fade = 255 - (deathTimer * 17);  // Fade over 15 frames
        int size = deathTimer / 5;

        auto sp = [matrix](int py, int px, int r, int g, int b) {
            if (px >= 0 && px < 64 && py >= 0 && py < 64)
                matrix->SetPixel(py, px, r, g, b);
        };

        // Explosion colors and size based on alien type
        int cr, cg, cb;   // Center color
        int er, eg, eb;   // Edge color
        int maxSize = 1;  // Explosion expand limit
        switch(type) {
            case BASIC:
                cr = 0; cg = fade; cb = fade;             // Cyan
                er = 0; eg = fade / 2; eb = fade;
                maxSize = 1;
                break;
            case FAST:
                cr = fade / 3; cg = fade / 2; cb = fade;  // Light blue
                er = fade / 4; eg = fade / 3; eb = fade / 2;
                maxSize = 1;
                break;
            case TANK:
                cr = fade * 3 / 4; cg = 0; cb = fade;    // Purple
                er = fade / 2; eg = 0; eb = fade / 2;
                maxSize = 3;
                break;
            case ELITE:
                cr = fade; cg = fade / 4; cb = fade / 2;  // Pink
                er = fade / 2; eg = fade / 6; eb = fade / 3;
                maxSize = 2;
                break;
            case LAUNCHER:
                cr = fade; cg = fade * 2 / 3; cb = 0;     // Orange
                er = fade / 2; eg = fade / 3; eb = 0;
                maxSize = 2;
                break;
            case BEAM:
                cr = fade / 3; cg = fade * 2 / 3; cb = fade;  // Cyan-blue
                er = fade / 4; eg = fade / 2; eb = fade / 2;
                maxSize = 2;
                break;
            case ANCHORED_ELITE:
                cr = fade; cg = fade / 4; cb = fade / 2;      // Pink (same as elite)
                er = fade / 2; eg = fade / 6; eb = fade / 3;
                maxSize = 2;
                break;
            case ROCKET_BOSS:
                cr = fade; cg = fade / 4; cb = 0;             // Orange-red
                er = fade / 2; eg = fade / 5; eb = 0;
                maxSize = 3;
                break;
            case MINI_BOSS:
                cr = 0; cg = fade; cb = fade / 2;         // Green-cyan
                er = 0; eg = fade / 2; eb = fade / 3;
                maxSize = 3;
                break;
            default:
                cr = 0; cg = fade; cb = fade;             // Cyan
                er = 0; eg = fade / 2; eb = fade;
                maxSize = 1;
                break;
        }

        if (size > maxSize) size = maxSize;

        // Center
        sp(y, x, cr, cg, cb);
        // Cardinal expanding ring
        if (size > 0) {
            sp(y - size, x, er, eg, eb);
            sp(y + size, x, er, eg, eb);
            sp(y, x - size, er, eg, eb);
            sp(y, x + size, er, eg, eb);
        }
        // Larger aliens get diagonal particles too
        if (size > 1) {
            sp(y - size, x - 1, er / 2, eg / 2, eb / 2);
            sp(y - size, x + 1, er / 2, eg / 2, eb / 2);
            sp(y + size, x - 1, er / 2, eg / 2, eb / 2);
            sp(y + size, x + 1, er / 2, eg / 2, eb / 2);
        }
        // Extra ring for tanks/mini-bosses
        if (maxSize >= 3 && deathTimer < 10) {
            sp(y - 1, x - 1, er, eg, eb);
            sp(y - 1, x + 1, er, eg, eb);
            sp(y + 1, x - 1, er, eg, eb);
            sp(y + 1, x + 1, er, eg, eb);
            sp(y - 2, x, er / 2, eg / 2, eb / 2);
            sp(y + 2, x, er / 2, eg / 2, eb / 2);
            sp(y, x - 2, er / 2, eg / 2, eb / 2);
            sp(y, x + 2, er / 2, eg / 2, eb / 2);
        }
        // Small diagonal flash for all types early on
        if (deathTimer < 5) {
            sp(y - 1, x - 1, er / 3, eg / 3, eb / 3);
            sp(y + 1, x + 1, er / 3, eg / 3, eb / 3);
        }
        return;
    }
    
    // Default alien rendering (used by base Alien class)
    // Subclasses should override this with their own colors
    int r = 0, g = 255, b = 0;  // Default: Green (BASIC)
    int r2 = 0, g2 = 200, b2 = 0;
    int r3 = 0, g3 = 150, b3 = 0;
    int r4 = 0, g4 = 180, b4 = 0;
    
    switch(type) {
        case BASIC:
            // Green
            r = 0; g = 255; b = 0;
            r2 = 0; g2 = 200; b2 = 0;
            r3 = 0; g3 = 150; b3 = 0;
            r4 = 0; g4 = 180; b4 = 0;
            break;
        case FAST:
            // Yellow
            r = 255; g = 255; b = 0;
            r2 = 200; g2 = 200; b2 = 0;
            r3 = 150; g3 = 150; b3 = 0;
            r4 = 180; g4 = 180; b4 = 0;
            break;
        case TANK:
            // Red
            r = 255; g = 0; b = 0;
            r2 = 200; g2 = 0; b2 = 0;
            r3 = 150; g3 = 0; b3 = 0;
            r4 = 180; g4 = 0; b4 = 0;
            break;
        case ELITE:
            // Purple
            r = 200; g = 0; b = 255;
            r2 = 150; g2 = 0; b2 = 200;
            r3 = 100; g3 = 0; b3 = 150;
            r4 = 130; g4 = 0; b4 = 180;
            break;
    }
    
    // Bounds check for 3x3 sprite (allow partial at bottom)
    if (x < 1 || x >= 63 || y < 1 || y > 66) return;

    auto safeSet = [matrix](int py, int px, int cr, int cg, int cb) {
        if (px >= 0 && px < 64 && py >= 0 && py < 64)
            matrix->SetPixel(py, px, cr, cg, cb);
    };

    safeSet(y, x, r, g, b);
    safeSet(y, x - 1, r2, g2, b2);
    safeSet(y, x + 1, r2, g2, b2);
    safeSet(y + 1, x - 1, r3, g3, b3);
    safeSet(y + 1, x, r4, g4, b4);
    safeSet(y + 1, x + 1, r3, g3, b3);
    safeSet(y - 1, x, r4, g4, b4);
}

void Alien::erase(RGBMatrix *matrix) {
    // Erase larger area to cover tank aliens and explosion particles
    // Add bounds checking to prevent out-of-bounds access
    for (int i = -4; i <= 4; i++) {
        for (int j = -4; j <= 4; j++) {
            int px = x + j;
            int py = y + i;
            // Only erase if within bounds
            if (px >= 0 && px < 64 && py >= 0 && py < 64) {
                matrix->SetPixel(py, px, 18, 10, 14);
            }
        }
    }
}

void Alien::updateDeathAnimation() {
    if (hitFlashTimer > 0) hitFlashTimer--;

    if (dying) {
        deathTimer++;
        if (deathTimer > 15) {  // Animation lasts 15 frames
            health = -1;  // Mark as completely dead
        }
    }
}

bool Alien::isDead() const {
    return health < 0;
}
