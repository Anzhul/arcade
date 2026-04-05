#ifndef INPUT_HPP
#define INPUT_HPP

struct InputState {
    int joystick_x;      // -100 to 100 (0 = center)
    int joystick_y;      // -100 to 100 (0 = center)
    int potentiometer;   // 0-100 range (shield/firerate ratio from proximity)
    int potentiometer2;  // 0-100 range (unused)
    bool button1;
    bool button2;
    bool button3;
    bool button4;        // Weapon toggle
};

// Abstract input provider interface
class InputProvider {
public:
    virtual ~InputProvider() = default;
    virtual bool read(InputState& state) = 0;  // Returns false on error/disconnect
};

#endif
