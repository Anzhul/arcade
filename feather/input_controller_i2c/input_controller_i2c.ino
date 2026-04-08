/*
 * Adafruit Feather M0 WiFi (ATWINC1500) Input Controller with I2C Sensors
 *
 * Hardware Setup:
 * - Button 1: Connect to Digital Pin 12 (with pullup)
 * - Button 2: Connect to Digital Pin 11 (with pullup)
 * - Button 3: Connect to Digital Pin 10 (with pullup)
 * - Button 4: Connect to Digital Pin 9 (with pullup)
 *
 * I2C Sensors (connected via SDA/SCL - STEMMA QT compatible):
 * - VCNL4200: Long Distance IR Proximity and Ambient Light Sensor (I2C addr: 0x51)
 * - LSM6DSOX: 6-DoF IMU - Accelerometer + Gyroscope (I2C addr: 0x6A)
 *
 * Board: Adafruit Feather M0 WiFi (ATSAMD21 + ATWINC1500)
 *
 * Libraries Required:
 * - WiFi101 (for ATWINC1500)
 * - Adafruit_VCNL4200
 * - Adafruit_LSM6DS (includes LSM6DSOX support)
 * - Adafruit_Sensor
 */

#include <WiFi101.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Adafruit_VCNL4200.h>
#include <Adafruit_LSM6DSOX.h>

// ===== WiFi Pin Configuration for M0 WiFi =====
#define WINC_CS   8
#define WINC_IRQ  7
#define WINC_RST  4
#define WINC_EN   2

// ===== WiFi Configuration =====
const char* ssid = "Ling";                         // Replace with your WiFi network name
const char* password = "Lingfamily";               // Replace with your WiFi password
const char* raspberryPiIP = "192.168.0.172";       // Replace with your Raspberry Pi's IP address
const int udpPort = 8888;                          // UDP port for sending data

WiFiUDP udp;

// ===== I2C Sensor Objects =====
Adafruit_VCNL4200 vcnl4200;
Adafruit_LSM6DSOX lsm6dsox;

// ===== Pin Definitions =====
// Digital Inputs (Buttons)
const int BUTTON1_PIN = 12;
const int BUTTON2_PIN = 11;
const int BUTTON3_PIN = 10;
const int BUTTON4_PIN = 9;

// ===== Sensor Data Structure =====
struct SensorData {
  // Buttons
  bool button1;
  bool button2;
  bool button3;
  bool button4;

  // VCNL4200 - Proximity and Ambient Light
  uint16_t proximity;
  uint16_t ambientLight;

  // LSM6DSOX - Accelerometer (m/s^2)
  float accelX;
  float accelY;
  float accelZ;

  // LSM6DSOX - Gyroscope (rad/s)
  float gyroX;
  float gyroY;
  float gyroZ;

  // LSM6DSOX - Temperature (C)
  float temperature;
};

SensorData currentData;
SensorData previousData;

// ===== Sensor Status =====
bool vcnl4200Found = false;
bool lsm6dsoxFound = false;

// ===== Settings =====
const int SEND_INTERVAL = 20;        // Send data every 20ms (50Hz)
const int DEBOUNCE_DELAY = 50;       // Button debounce time in milliseconds
const int PROX_THRESHOLD = 50;       // Minimum proximity change to trigger update
const int LIGHT_THRESHOLD = 20;      // Minimum light change to trigger update
const float ACCEL_THRESHOLD = 0.1;   // Minimum acceleration change (m/s^2)
const float GYRO_THRESHOLD = 0.05;   // Minimum gyro change (rad/s)

// ===== Debug/Logging Settings =====
const bool DEBUG_BUTTONS = true;     // Log button state changes
const bool DEBUG_VCNL4200 = true;    // Log proximity/light readings
const bool DEBUG_LSM6DSOX = true;    // Log IMU readings
const bool DEBUG_WIFI = true;        // Log WiFi status
const unsigned long STATUS_INTERVAL = 5000;  // Print status every 5 seconds

unsigned long lastSendTime = 0;
unsigned long lastStatusTime = 0;
unsigned long lastTiltPrintTime = 0;
const unsigned long TILT_PRINT_INTERVAL = 100;  // Print tilt data every 100ms
unsigned long loopCount = 0;
unsigned long udpPacketsSent = 0;
unsigned long lastDebounceTime[4] = {0, 0, 0, 0};
bool lastButtonState[4] = {HIGH, HIGH, HIGH, HIGH};
bool buttonState[4] = {HIGH, HIGH, HIGH, HIGH};

// ===== Function Prototypes =====
void setupWiFi();
void setupI2CSensors();
void scanI2CBus();
void readSensors();
void sendData();
bool dataChanged();
void printStatus();

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== Feather I2C Input Controller ===");

  // Configure button pins with internal pullup resistors
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  pinMode(BUTTON4_PIN, INPUT_PULLUP);

  // Initialize I2C
  Wire.begin();
  Serial.println("[I2C] Bus initialized");

  // Scan I2C bus for devices
  scanI2CBus();

  // Initialize sensor data
  memset(&currentData, 0, sizeof(currentData));
  memset(&previousData, 0, sizeof(previousData));

  // Setup I2C sensors
  setupI2CSensors();

  // Connect to WiFi
  setupWiFi();

  // Begin UDP
  udp.begin(udpPort);

  Serial.println("Setup complete. Starting to read sensors...\n");
}

void loop() {
  unsigned long currentTime = millis();
  loopCount++;

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

  // Print tilt data every 100ms
  if (lsm6dsoxFound && currentTime - lastTiltPrintTime >= TILT_PRINT_INTERVAL) {
    Serial.print("[TILT] Accel: ");
    Serial.print(currentData.accelX, 2);
    Serial.print(", ");
    Serial.print(currentData.accelY, 2);
    Serial.print(", ");
    Serial.print(currentData.accelZ, 2);
    Serial.print(" | Gyro: ");
    Serial.print(currentData.gyroX, 3);
    Serial.print(", ");
    Serial.print(currentData.gyroY, 3);
    Serial.print(", ");
    Serial.println(currentData.gyroZ, 3);
    lastTiltPrintTime = currentTime;
  }

  // Print periodic status update
  if (currentTime - lastStatusTime >= STATUS_INTERVAL) {
    printStatus();
    lastStatusTime = currentTime;
  }
}

void scanI2CBus() {
  Serial.println("[I2C] Scanning for devices...");
  int deviceCount = 0;

  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("[I2C] Device found at 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);

      // Identify known devices
      if (address == 0x51) {
        Serial.print(" (VCNL4200 - Proximity/Light)");
      } else if (address == 0x6A) {
        Serial.print(" (LSM6DSOX - IMU)");
      } else if (address == 0x6B) {
        Serial.print(" (LSM6DSOX - IMU, alt addr)");
      }
      Serial.println();
      deviceCount++;
    }
  }

  if (deviceCount == 0) {
    Serial.println("[I2C] WARNING: No devices found! Check wiring.");
  } else {
    Serial.print("[I2C] Scan complete. Found ");
    Serial.print(deviceCount);
    Serial.println(" device(s).");
  }
  Serial.println();
}

void setupI2CSensors() {
  // Initialize VCNL4200 - Proximity and Ambient Light Sensor
  Serial.println("[VCNL4200] Initializing proximity/light sensor (addr: 0x51)...");
  if (vcnl4200.begin()) {
    vcnl4200Found = true;
    Serial.println("[VCNL4200] SUCCESS - Sensor initialized!");

    // Enable sensors (they start in shutdown mode!)
    vcnl4200.setProxShutdown(false);  // Enable proximity sensor
    vcnl4200.setALSshutdown(false);   // Enable ambient light sensor

    // Configure VCNL4200 settings
    vcnl4200.setProxLEDCurrent(VCNL4200_LED_I_200MA);
    vcnl4200.setProxIntegrationTime(VCNL4200_PS_IT_8T);
    vcnl4200.setALSIntegrationTime(VCNL4200_ALS_IT_100MS);

    // Read initial values
    uint16_t prox = vcnl4200.readProxData();
    uint16_t light = vcnl4200.readALSdata();
    Serial.print("[VCNL4200] Initial reading - Proximity: ");
    Serial.print(prox);
    Serial.print(", Light: ");
    Serial.println(light);
  } else {
    Serial.println("[VCNL4200] FAILED - Sensor not found! Check wiring.");
  }

  // Initialize LSM6DSOX - 6-DoF IMU
  Serial.println("[LSM6DSOX] Initializing IMU (addr: 0x6A)...");
  if (lsm6dsox.begin_I2C()) {
    lsm6dsoxFound = true;
    Serial.println("[LSM6DSOX] SUCCESS - Sensor initialized!");

    // Configure LSM6DSOX settings
    lsm6dsox.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);
    lsm6dsox.setGyroRange(LSM6DS_GYRO_RANGE_500_DPS);
    lsm6dsox.setAccelDataRate(LSM6DS_RATE_104_HZ);
    lsm6dsox.setGyroDataRate(LSM6DS_RATE_104_HZ);

    Serial.println("[LSM6DSOX] Config: Accel +/-4G, Gyro +/-500DPS, Rate 104Hz");

    // Read initial values
    sensors_event_t accel, gyro, temp;
    lsm6dsox.getEvent(&accel, &gyro, &temp);
    Serial.print("[LSM6DSOX] Initial reading - Accel X:");
    Serial.print(accel.acceleration.x, 2);
    Serial.print(" Y:");
    Serial.print(accel.acceleration.y, 2);
    Serial.print(" Z:");
    Serial.print(accel.acceleration.z, 2);
    Serial.print(" m/s^2, Temp: ");
    Serial.print(temp.temperature, 1);
    Serial.println(" C");
  } else {
    Serial.println("[LSM6DSOX] FAILED - Sensor not found! Check wiring.");
  }

  Serial.println();
}

void setupWiFi() {
  Serial.println("[WiFi] Setting up WiFi module...");

  // Manually set WiFi pins for ATWINC1500
  WiFi.setPins(WINC_CS, WINC_IRQ, WINC_RST, WINC_EN);
  Serial.println("[WiFi] Pins configured (CS:8, IRQ:7, RST:4, EN:2)");

  // Check for the WiFi module
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("[WiFi] ERROR: WiFi module not detected!");
    Serial.println("[WiFi] Check that ATWINC1500 is properly connected.");
    while (true); // Don't continue
  }
  Serial.println("[WiFi] Module detected!");

  Serial.print("[WiFi] Connecting to SSID: ");
  Serial.println(ssid);

  // Connect to WPA/WPA2 network
  int status = WL_IDLE_STATUS;
  int attempts = 0;

  while (status != WL_CONNECTED && attempts < 30) {
    Serial.print("[WiFi] Attempt ");
    Serial.print(attempts + 1);
    Serial.print("/30...");
    status = WiFi.begin(ssid, password);
    delay(500);

    if (status == WL_CONNECTED) {
      Serial.println(" Connected!");
    } else {
      Serial.println(" Failed, retrying...");
    }
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] === CONNECTION SUCCESSFUL ===");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("[WiFi] Signal strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.print("[WiFi] Target: ");
    Serial.print(raspberryPiIP);
    Serial.print(":");
    Serial.println(udpPort);
  } else {
    Serial.println("[WiFi] === CONNECTION FAILED ===");
    Serial.println("[WiFi] Check SSID and password.");
  }
  Serial.println();
}

void readSensors() {
  // Read buttons with debouncing
  int buttonPins[4] = {BUTTON1_PIN, BUTTON2_PIN, BUTTON3_PIN, BUTTON4_PIN};
  bool* buttonResults[4] = {&currentData.button1, &currentData.button2,
                            &currentData.button3, &currentData.button4};
  bool previousButtonStates[4] = {currentData.button1, currentData.button2,
                                   currentData.button3, currentData.button4};

  for (int i = 0; i < 4; i++) {
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

    // Log button state changes
    if (DEBUG_BUTTONS && *buttonResults[i] != previousButtonStates[i]) {
      Serial.print("[BTN] Button ");
      Serial.print(i + 1);
      Serial.print(" (pin ");
      Serial.print(buttonPins[i]);
      Serial.print("): ");
      Serial.println(*buttonResults[i] ? "PRESSED" : "RELEASED");
    }
  }

  // Read VCNL4200 - Proximity and Ambient Light
  if (vcnl4200Found) {
    currentData.proximity = vcnl4200.readProxData();
    currentData.ambientLight = vcnl4200.readALSdata();
  }

  // Read LSM6DSOX - IMU data
  if (lsm6dsoxFound) {
    sensors_event_t accel, gyro, temp;
    lsm6dsox.getEvent(&accel, &gyro, &temp);

    currentData.accelX = accel.acceleration.x;
    currentData.accelY = accel.acceleration.y;
    currentData.accelZ = accel.acceleration.z;

    currentData.gyroX = gyro.gyro.x;
    currentData.gyroY = gyro.gyro.y;
    currentData.gyroZ = gyro.gyro.z;

    currentData.temperature = temp.temperature;
  }
}

bool dataChanged() {
  // Check if any button state changed
  if (currentData.button1 != previousData.button1 ||
      currentData.button2 != previousData.button2 ||
      currentData.button3 != previousData.button3 ||
      currentData.button4 != previousData.button4) {
    return true;
  }

  // Check VCNL4200 changes
  if (vcnl4200Found) {
    if (abs((int)currentData.proximity - (int)previousData.proximity) > PROX_THRESHOLD ||
        abs((int)currentData.ambientLight - (int)previousData.ambientLight) > LIGHT_THRESHOLD) {
      return true;
    }
  }

  // Check LSM6DSOX changes
  if (lsm6dsoxFound) {
    if (abs(currentData.accelX - previousData.accelX) > ACCEL_THRESHOLD ||
        abs(currentData.accelY - previousData.accelY) > ACCEL_THRESHOLD ||
        abs(currentData.accelZ - previousData.accelZ) > ACCEL_THRESHOLD ||
        abs(currentData.gyroX - previousData.gyroX) > GYRO_THRESHOLD ||
        abs(currentData.gyroY - previousData.gyroY) > GYRO_THRESHOLD ||
        abs(currentData.gyroZ - previousData.gyroZ) > GYRO_THRESHOLD) {
      return true;
    }
  }

  return false;
}

void sendData() {
  // Create JSON-formatted string for easy parsing on Raspberry Pi
  String jsonData = "{";

  // Buttons
  jsonData += "\"btn1\":" + String(currentData.button1 ? 1 : 0) + ",";
  jsonData += "\"btn2\":" + String(currentData.button2 ? 1 : 0) + ",";
  jsonData += "\"btn3\":" + String(currentData.button3 ? 1 : 0) + ",";
  jsonData += "\"btn4\":" + String(currentData.button4 ? 1 : 0);

  // VCNL4200 data
  if (vcnl4200Found) {
    jsonData += ",\"prox\":" + String(currentData.proximity);
    jsonData += ",\"light\":" + String(currentData.ambientLight);
  }

  // LSM6DSOX data
  if (lsm6dsoxFound) {
    jsonData += ",\"accelX\":" + String(currentData.accelX, 2);
    jsonData += ",\"accelY\":" + String(currentData.accelY, 2);
    jsonData += ",\"accelZ\":" + String(currentData.accelZ, 2);
    jsonData += ",\"gyroX\":" + String(currentData.gyroX, 3);
    jsonData += ",\"gyroY\":" + String(currentData.gyroY, 3);
    jsonData += ",\"gyroZ\":" + String(currentData.gyroZ, 3);
    jsonData += ",\"temp\":" + String(currentData.temperature, 1);
  }

  jsonData += "}";

  // Send UDP packet to Raspberry Pi
  if (WiFi.status() == WL_CONNECTED) {
    udp.beginPacket(raspberryPiIP, udpPort);
    udp.print(jsonData);
    udp.endPacket();
    udpPacketsSent++;

    // Debug output (comment out for better performance)
    // Serial.println(jsonData);
  } else {
    if (DEBUG_WIFI) {
      Serial.println("[WiFi] WARNING: Disconnected! Cannot send data.");
    }
  }
}

void printStatus() {
  // Buttons
  Serial.print("BTN: ");
  Serial.print(currentData.button1 ? "1" : "0");
  Serial.print(currentData.button2 ? "1" : "0");
  Serial.print(currentData.button3 ? "1" : "0");
  Serial.print(currentData.button4 ? "1" : "0");

  // Proximity and Light
  Serial.print(" | Prox: ");
  Serial.print(currentData.proximity);
  Serial.print(" Light: ");
  Serial.print(currentData.ambientLight);

  // Accelerometer
  Serial.print(" | Accel: ");
  Serial.print(currentData.accelX, 2);
  Serial.print(",");
  Serial.print(currentData.accelY, 2);
  Serial.print(",");
  Serial.print(currentData.accelZ, 2);

  // Gyroscope
  Serial.print(" | Gyro: ");
  Serial.print(currentData.gyroX, 3);
  Serial.print(",");
  Serial.print(currentData.gyroY, 3);
  Serial.print(",");
  Serial.println(currentData.gyroZ, 3);
}
