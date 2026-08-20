#include <Arduino.h>
#include <Wire.h>

#include "app_state.h"
#include "config.h"
#include "watering_controller.h"

void requestDataFromArduino() {
  Wire.requestFrom(ARDUINO_I2C_ADDR, 32);
  String response;
  while (Wire.available()) response += static_cast<char>(Wire.read());

  if (response.length() == 0) {
    Serial.println("-> Loi: Khong nhan duoc du lieu tu Arduino!");
    return;
  }

  Serial.print("Data from Arduino: ");
  Serial.println(response);
  SensorData receivedData;
  const int parsed = sscanf(response.c_str(), "%f,%f,%d,%d,%d,%d",
                            &receivedData.temperature, &receivedData.humidity,
                            &receivedData.light, &receivedData.soil,
                            &receivedData.water, &receivedData.button);
  if (parsed != 6) {
    Serial.println("-> Loi: Du lieu nhan duoc khong khop dinh dang!");
    return;
  }

  sensorData = receivedData;
  Serial.println("-> Tach chuoi thanh cong!");
  Serial.printf("Temp: %.1f C | Hum: %.1f %% | Light: %d %% | Soil: %d %% | Water: %d %% | Button: %d\n",
                sensorData.temperature, sensorData.humidity, sensorData.light,
                sensorData.soil, sensorData.water, sensorData.button);
  checkWateringCondition();
}

void sendCommandToArduino(const char* command) {
  Serial.print("Sending command to Arduino: ");
  Serial.println(command);
  Wire.beginTransmission(ARDUINO_I2C_ADDR);
  Wire.write(command);
  const byte error = Wire.endTransmission();
  if (error == 0) {
    Serial.println("-> Gui lenh thanh cong!");
  } else {
    Serial.print("-> Loi I2C: ");
    Serial.println(error);
  }
}
