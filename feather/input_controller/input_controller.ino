 /*
 * Adafruit Feather M0 WiFi (ATWINC1500) Input Controller
 *
 * Hardware Setup:
 * - Analog Joystick (2 axes): Connect to A0 (X-axis) and A1 (Y-axis)
 * - Potentiometer 1: Connect to A2
 * - Potentiometer 2: Connect to A3
 * - Button 1: Connect to Digital Pin 5 (with pullup)
 * - Button 2: Connect to Digital Pin 6 (with pullup)
 * - Button 3: Connect to Digital Pin 9 (with pullup)
 *
 * All analog inputs should have ground and 3.3V connections
 * Buttons should be wired between the pin and GND (internal pullup enabled)
 *
 * Board: Adafruit Feather M0 WiFi (ATSAMD21 + ATWINC1500)
 */

#include <WiFi101.h>
#include <WiFiUdp.h>

// ===== WiFi Configuration =====
const char* ssid = "Ling";          // Replace with your WiFi network name
const char* password = "Lingfamily";   // Replace with your WiFi password
const char* raspberryPiIP = "192.168.0.172";   // Replace with your Raspberry Pi's IP address
const int udpPort = 8888;                      // UDP port for sending data

WiFiUDP udp;

// ===== Pin Definitions =====
// Analog Inputs (10-bit ADC on M0: 0-1023)
const int JOYSTICK_X_PIN = A0;
const int JOYSTICK_Y_PIN = A1;
const int POT1_PIN = A2;
const int POT2_PIN = A3;

// Digital Inputs (Buttons)
const int BUTTON1_PIN = 5;
const int BUTTON2_PIN = 6;
const int BUTTON3_PIN = 9;

// ===== Sensor Values =====
struct SensorData {
  int joystickX;
  int joystickY;
  int pot1;
  int pot2;
  bool button1;
  bool button2;
  bool button3;
};

SensorData currentData;
SensorData previousData;

// ===== Settings =====
const int SEND_INTERVAL = 20;        // Send data every 20ms (50Hz)
const int DEBOUNCE_DELAY = 50;       // Button debounce time in milliseconds
const int ANALOG_THRESHOLD = 10;     // Minimum analog change to trigger update

unsigned long lastSendTime = 0;
unsigned long lastDebounceTime[3] = {0, 0, 0};
bool lastButtonState[3] = {HIGH, HIGH, HIGH};
bool buttonState[3] = {HIGH, HIGH, HIGH};

// ===== Function Prototypes =====
void setupWiFi();
void readSensors();
void sendData();
bool dataChanged();
int mapAnalog(int value);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== Feather Input Controller ===");

  // Configure button pins with internal pullup resistors
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);

  // Initialize sensor data
  memset(&currentData, 0, sizeof(currentData));
  memset(&previousData, 0, sizeof(previousData));

  // Connect to WiFi
  setupWiFi();

  // Begin UDP
  udp.begin(udpPort);

  Serial.println("Setup complete. Starting to read sensors...\n");
}

void loop() {
  unsigned long currentTime = millis();

  // Read all sensors
  readSensors();

  // Send data at regular intervals or when significant changes occur
  if (currentTime - lastSendTime >= SEND_INTERVAL) {
    if (dataChanged()) {
      sendData();
      previousData = currentData;
    }
    lastSendTime = currentTime;
  }
}

void setupWiFi() {
  // Check for the WiFi module
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi module not detected!");
    while (true); // Don't continue
  }

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  // Connect to WPA/WPA2 network
  int status = WL_IDLE_STATUS;
  int attempts = 0;

  while (status != WL_CONNECTED && attempts < 30) {
    status = WiFi.begin(ssid, password);
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Sending data to: ");
    Serial.print(raspberryPiIP);
    Serial.print(":");
    Serial.println(udpPort);
  } else {
    Serial.println("\nWiFi connection failed!");
    Serial.println("Please check your credentials and try again.");
  }
}

void readSensors() {
  // Read analog inputs (10-bit ADC: 0-1023 on M0)
  currentData.joystickX = analogRead(JOYSTICK_X_PIN);
  currentData.joystickY = analogRead(JOYSTICK_Y_PIN);
  currentData.pot1 = analogRead(POT1_PIN);
  currentData.pot2 = analogRead(POT2_PIN);

  // Read buttons with debouncing
  int buttonPins[3] = {BUTTON1_PIN, BUTTON2_PIN, BUTTON3_PIN};
  bool* buttonResults[3] = {&currentData.button1, &currentData.button2, &currentData.button3};

  for (int i = 0; i < 3; i++) {
    bool reading = digitalRead(buttonPins[i]);

    // If the button state changed, reset debounce timer
    if (reading != lastButtonState[i]) {
      lastDebounceTime[i] = millis();
    }

    // If enough time has passed, accept the new state
    if ((millis() - lastDebounceTime[i]) > DEBOUNCE_DELAY) {
      if (reading != buttonState[i]) {
        buttonState[i] = reading;
      }
    }

    // Store button state (inverted because of pullup - LOW means pressed)
    *buttonResults[i] = (buttonState[i] == LOW);
    lastButtonState[i] = reading;
  }
}

bool dataChanged() {
  // Check if any button state changed
  if (currentData.button1 != previousData.button1 ||
      currentData.button2 != previousData.button2 ||
      currentData.button3 != previousData.button3) {
    return true;
  }

  // Check if analog values changed significantly
  if (abs(currentData.joystickX - previousData.joystickX) > ANALOG_THRESHOLD ||
      abs(currentData.joystickY - previousData.joystickY) > ANALOG_THRESHOLD ||
      abs(currentData.pot1 - previousData.pot1) > ANALOG_THRESHOLD ||
      abs(currentData.pot2 - previousData.pot2) > ANALOG_THRESHOLD) {
    return true;
  }

  return false;
}

void sendData() {
  // Create JSON-formatted string for easy parsing on Raspberry Pi
  String jsonData = "{";
  jsonData += "\"joyX\":" + String(currentData.joystickX) + ",";
  jsonData += "\"joyY\":" + String(currentData.joystickY) + ",";
  jsonData += "\"pot1\":" + String(currentData.pot1) + ",";
  jsonData += "\"pot2\":" + String(currentData.pot2) + ",";
  jsonData += "\"btn1\":" + String(currentData.button1 ? 1 : 0) + ",";
  jsonData += "\"btn2\":" + String(currentData.button2 ? 1 : 0) + ",";
  jsonData += "\"btn3\":" + String(currentData.button3 ? 1 : 0);
  jsonData += "}";

  // Send UDP packet to Raspberry Pi
  if (WiFi.status() == WL_CONNECTED) {
    udp.beginPacket(raspberryPiIP, udpPort);
    udp.print(jsonData);
    udp.endPacket();

    // Debug output (comment out for better performance)
    Serial.println(jsonData);
  } else {
    Serial.println("WiFi disconnected!");
    // Note: WiFi101 doesn't support auto-reconnect
    // You may need to restart the board if connection is lost
  }
}
