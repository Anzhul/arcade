
#include <iostream>
#include "led-matrix.h"
#include "graphics.h"
#include <csignal>
#include "ship.hpp"
#include "game.hpp"
#include <wiringPi.h> // Include WiringPi library!
#include <wiringPiI2C.h>
#include <unistd.h> // Needed for sleep
#include <math.h>
using namespace std;

#define CHANNEL_0 0x84 // Single-ended CH0
#define CHANNEL_1 0xC4 // Single-ended CH1
#define CHANNEL_2 0x94 // Single-ended CH2
#define CHANNEL_3 0xD4 // Single-ended CH3
#define CHANNEL_4 0xA4 // Single-ended CH4
#define CHANNEL_5 0xE4 // Single-ended CH5
#define CHANNEL_6 0xB4 // Single-ended CH6
#define CHANNEL_7 0xF4 // Single-ended CH7

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
  //matrix_options.brightness = 75;
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

  Game game = Game();
  game.setup();

  for (int row = 0; row < 64; ++row){
    for (int col = 0; col < 64; ++col){
      matrix->SetPixel(col, row, 18, 10, 14);
    }
  }

  int clock = 0;

  while (!interrupt_received){
    int xValue = readADC(CHANNEL_0, adc); // Read X-axis (Channel 0)
    int yValue = readADC(CHANNEL_1, adc); // Read Y-axis (Channel 1)
    int pValue = readADC(CHANNEL_2, adc); // Read P-meter (Channel 2)
    int button1 = readADC(CHANNEL_3, adc); // Read BUtton1 (Channel 3)
    int button2 = readADC(CHANNEL_5, adc); // Read BUtton2 (Channel 4)
    int button3 = readADC(CHANNEL_7, adc); // Read BUtton3 (Channel 5)
    /*
    std::cout << "Joystick X: " << xValue << ", Y: " << yValue
              << ", Pmeter: " << pValue << ", Button1: " << button1 <<
              ", Button2: " << button2 << ", BUtton3: " << button3 <<std::endl;
    */

    game.update(xValue, yValue, pValue, button1, button2, button3, matrix, clock);

    usleep(100); // Sleep for 100ms (adjust as necessary)
    ++clock;
  }
  matrix->Clear();
  delete matrix;
  return 0;
}