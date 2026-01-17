#include <SPI.h>
#include <WiFi101.h>

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  Serial.println("=== Manual WiFi Initialization Test ===");
  
  // Manually configure WiFi pins for Feather M0 WiFi
  WiFi.setPins(8, 7, 4, 2);  // CS, IRQ, RST, EN
  
  delay(100);
  
  Serial.println("WiFi pins manually configured.");
  Serial.println("Checking WiFi module...");
  
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("STILL NOT DETECTED - Possible hardware issue");
  } else {
    Serial.println("SUCCESS! WiFi module detected!");
    Serial.print("Firmware version: ");
    Serial.println(WiFi.firmwareVersion());
  }
}

void loop() {}
