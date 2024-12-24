
#include <iostream>
#include <wiringPi.h> // Include WiringPi library!
#include <wiringPiI2C.h>
#include <unistd.h>  // Needed for sleep
using namespace std;
#define CHANNEL_0 0x84 // Single-ended CH0
#define CHANNEL_1 0xC4 // Single-ended CH1
#define CHANNEL_2 0x94 // Single-ended CH2
#define CHANNEL_3 0xD4 // Single-ended CH3

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

int readADC(int channel, int adc) {

    wiringPiI2CWrite(adc, channel);     // Send control byte
    usleep(100);                           // Short delay for conversion
    return wiringPiI2CRead(adc);    
}

int main(void)
{
  // uses BCM numbering of the GPIOs and directly accesses the GPIO registers.
  wiringPiSetupGpio();

    // Initialize I2C (ADS7830)
    int adc = wiringPiI2CSetup(0x4b);  // Replace with your ADC's I2C address

    while (true) {
        int xValue = readADC(CHANNEL_0, adc);  // Read X-axis (Channel 0)
        int yValue = readADC(CHANNEL_1, adc);  // Read Y-axis (Channel 1)
        int pValue = readADC(CHANNEL_2, adc);  // Read P-meter (Channel 2)

        std::cout << "Joystick X: " << xValue << ", Y: " << yValue 
        <<", Pmeter"<< pValue << std::endl;

        usleep(10000);  // Sleep for 100ms (adjust as necessary)
    }

    return 0;
}