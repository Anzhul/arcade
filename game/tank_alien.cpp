#include "tank_alien.hpp"
#include <algorithm>  // for std::max, std::min

TankAlien::TankAlien() : Alien(60, 1, 32, 5, TANK) {
    // Ensure position is valid for tank size (7x7)
    setX(std::max(3, std::min(60, get_x())));
    setY(std::max(3, std::min(60, get_y())));
}

TankAlien::TankAlien(int x_in, int y_in) : Alien(60, 1, x_in, y_in, TANK) {
    // Ensure position is valid for tank size (7x7)
    setX(std::max(3, std::min(60, x_in)));
    setY(std::max(3, std::min(60, y_in)));
}

TankAlien::~TankAlien() {
}

void TankAlien::draw(RGBMatrix *matrix) {
    if (get_health() < 0) return;  // Completely dead, don't draw
    
    int x = get_x();
    int y = get_y();
    
    // Bounds check - tank is 7x7, so check extended bounds
    if (x < 3 || x >= 61 || y < 3 || y >= 61) return;
    
    if (get_health() == 0) {
        // Use parent class death animation
        Alien::draw(matrix);
        return;
    }
    
    // Red tank alien sprite (large 7x7 pixels for heavily armored look)
    // Center core (3x3)
    matrix->SetPixel(y, x, 255, 0, 0);
    matrix->SetPixel(y, x - 1, 220, 0, 0);
    matrix->SetPixel(y, x + 1, 220, 0, 0);
    matrix->SetPixel(y - 1, x, 220, 0, 0);
    matrix->SetPixel(y + 1, x, 220, 0, 0);
    matrix->SetPixel(y - 1, x - 1, 200, 0, 0);
    matrix->SetPixel(y - 1, x + 1, 200, 0, 0);
    matrix->SetPixel(y + 1, x - 1, 200, 0, 0);
    matrix->SetPixel(y + 1, x + 1, 200, 0, 0);
    
    // Outer armor layer
    matrix->SetPixel(y - 2, x, 180, 0, 0);
    matrix->SetPixel(y + 2, x, 180, 0, 0);
    matrix->SetPixel(y, x - 2, 180, 0, 0);
    matrix->SetPixel(y, x + 2, 180, 0, 0);
    
    // Corner armor plates
    matrix->SetPixel(y - 2, x - 1, 150, 0, 0);
    matrix->SetPixel(y - 2, x + 1, 150, 0, 0);
    matrix->SetPixel(y + 2, x - 1, 150, 0, 0);
    matrix->SetPixel(y + 2, x + 1, 150, 0, 0);
    matrix->SetPixel(y - 1, x - 2, 150, 0, 0);
    matrix->SetPixel(y + 1, x - 2, 150, 0, 0);
    matrix->SetPixel(y - 1, x + 2, 150, 0, 0);
    matrix->SetPixel(y + 1, x + 2, 150, 0, 0);
    
    // Extra heavy armor points
    matrix->SetPixel(y - 3, x, 120, 0, 0);
    matrix->SetPixel(y + 3, x, 120, 0, 0);
    matrix->SetPixel(y, x - 3, 120, 0, 0);
    matrix->SetPixel(y, x + 3, 120, 0, 0);
}
