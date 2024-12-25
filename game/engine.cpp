
#include <iostream>
#include "led-matrix.h"
#include "graphics.h"
#include <csignal>
#include "ship.hpp"
#include <wiringPi.h> // Include WiringPi library!
#include <wiringPiI2C.h>
#include <unistd.h> // Needed for sleep
#include <math.h>
using namespace std;

#define CHANNEL_0 0x84 // Single-ended CH0
#define CHANNEL_1 0xC4 // Single-ended CH1
#define CHANNEL_2 0x94 // Single-ended CH2
#define CHANNEL_3 0xD4 // Single-ended CH3

using namespace rgb_matrix;

volatile bool interrupt_received = false;

// Function to handle Ctrl+C
void InterruptHandler(int signo)
{
  interrupt_received = true;
}

void loop()
{
  // Smooth the readings from an analog input
  int total = 0;
  int num_readings = 10;
  for (int i = 0; i < num_readings; i++)
  {
    // total += analogRead(POTENTIOMETER_PIN_NUMBER);
  }
  int potentiometer_value = total / num_readings;

  // bool button_pressed = (digitalRead(BUTTON_PIN_NUMBER) == HIGH);

  // game.update(potentiometer_value, button_pressed);

  delay(25);
}

int readADC(int channel, int adc)
{
  wiringPiI2CWrite(adc, channel); // Send control byte
  usleep(100);                    // Short delay for conversion
  return wiringPiI2CRead(adc);
}

int main(void)
{
  // uses BCM numbering of the GPIOs and directly accesses the GPIO registers.
  wiringPiSetup();


  // Initialize I2C (ADS7830)
  int adc = wiringPiI2CSetup(0x4b); // Replace with your ADC's I2C address
  // Options for the LED matrix
  RGBMatrix::Options matrix_options;
  matrix_options.rows = 64;
  matrix_options.cols = 64;
  matrix_options.chain_length = 1;
  matrix_options.parallel = 1;
  matrix_options.brightness = 75;
  matrix_options.hardware_mapping = "adafruit-hat";

  RuntimeOptions runtime_options;
  runtime_options.gpio_slowdown = 4;

  // Create the matrix
  RGBMatrix *matrix = CreateMatrixFromOptions(matrix_options, runtime_options);
  if (matrix == nullptr)
  {
    std::cerr << "Could not create matrix. Exiting." << std::endl;
    return 1;
  }

  // Fill the entire matrix with the color
  for (int row = 0; row < 64; ++row)
  {
    for (int col = 0; col < 64; ++col)
    {
      matrix->SetPixel(col, row, 0, 150, 0);
    }
  }

  Ship player;
  Ship Ship(100, 1, 64 / 2, 0);

  while (!interrupt_received){
    int xValue = readADC(0x84, adc); // Read X-axis (Channel 0)
    int yValue = readADC(0xC4, adc); // Read Y-axis (Channel 1)
    int pValue = readADC(0x94, adc); // Read P-meter (Channel 2)

    std::cout << "Joystick X: " << xValue << ", Y: " << yValue
              << ", Pmeter" << pValue << std::endl;

    for (int row = 0; row < 64; ++row){
      for (int col = 0; col < 64; ++col){
        matrix->SetPixel(col, row, xValue, 150, yValue);
      }
    }

    usleep(1000); // Sleep for 100ms (adjust as necessary)
  }
  // matrix->Clear();
  // delete matrix;
  return 0;
}