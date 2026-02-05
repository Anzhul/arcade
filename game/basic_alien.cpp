#include "basic_alien.hpp"

BasicAlien::BasicAlien() : Alien(30, 1, 32, 5, BASIC) {
}

BasicAlien::BasicAlien(int x_in, int y_in) : Alien(30, 1, x_in, y_in, BASIC) {
}

BasicAlien::~BasicAlien() {
}

void BasicAlien::draw(RGBMatrix *matrix) {
    if (get_health() < 0) return;  // Completely dead, don't draw
    
    int x = get_x();
    int y = get_y();
    
    // Bounds check - basic alien is 3x3
    if (x < 1 || x >= 63 || y < 1 || y >= 63) return;
    
    if (get_health() == 0) {
        // Use parent class death animation
        Alien::draw(matrix);
        return;
    }
    
    // Green basic alien sprite (3x3 pixels)
    // Center
    matrix->SetPixel(y, x, 0, 255, 0);
    
    // Arms/wings
    matrix->SetPixel(y, x - 1, 0, 200, 0);
    matrix->SetPixel(y, x + 1, 0, 200, 0);
    
    // Bottom
    matrix->SetPixel(y + 1, x - 1, 0, 150, 0);
    matrix->SetPixel(y + 1, x, 0, 180, 0);
    matrix->SetPixel(y + 1, x + 1, 0, 150, 0);
    
    // Top
    matrix->SetPixel(y - 1, x, 0, 180, 0);
}
