#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "i2c_service.h"
#include "mqtt_service.h"

namespace {
unsigned long lastStatusTime = 0;
}

void setup() {
  Serial.begin(115200);
  randomSeed(micros());
  Wire.begin(D1, D2);
  setupWifi();
  setupMqtt();
}

void loop() {
  maintainMqttConnection();
  mqttLoop();

  const unsigned long currentMillis = millis();
  if (currentMillis - lastStatusTime >= STATUS_INTERVAL) {
    lastStatusTime = currentMillis;
    requestDataFromArduino();
    publishStatus();
  }
}
