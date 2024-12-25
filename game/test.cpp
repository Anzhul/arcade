
#include <iostream>
#include <wiringPi.h> // Include WiringPi library!
#include <wiringPiI2C.h>
#include <unistd.h> // Needed for sleep
#include "led-matrix.h"
#include "graphics.h"
#include <csignal>
#include "ship.hpp"

int main()
{
    // Initialize WiringPi
    if (wiringPiSetup() == -1)
    {
        return 1;
    }
    pinMode(0, OUTPUT); // Example GPIO pin
    digitalWrite(0, HIGH);

    Ship player = Ship(100, 1, 0, 0);

    // Initialize RGBMatrix
    rgb_matrix::RuntimeOptions options;
    options.gpio_slowdown = 2; // Example: Slow down GPIO to avoid conflicts

    return 0;
}