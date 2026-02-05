#ifndef FAST_ALIEN_HPP
#define FAST_ALIEN_HPP

#include "alien.hpp"

class FastAlien : public Alien {
public:
    FastAlien();
    FastAlien(int x, int y);
    ~FastAlien();
    
    void draw(RGBMatrix *matrix) override;
    void move(int dx, int dy) override;
    
private:
    int dashThreshold;  // Y position where dash activates
    bool hasDashed;     // Whether dash has been used
    int dashCounter;    // Counter for dash duration
};

#endif
