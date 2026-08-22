#include <Arduino.h>
#include <Wire.h> // Gọi lại thư viện Wire

#include "app_state.h"
#include "config.h"
#include "watering_controller.h"

void requestDataFromArduino() {
  Wire.requestFrom(ARDUINO_I2C_ADDR, 40); // Đọc 40 bytes khớp với kích thước i2cData
  String response = "";
  
  while (Wire.available()) {
    char c = Wire.read();
    // Lọc ký tự rỗng của I2C để sscanf không bị lỗi
    if (c != 255 && c != '\0' && isPrintable(c)) {
      response += c;
    }
  }

  response.trim();
  if (response.length() == 0) return;

  SensorData receivedData;
  const int parsed = sscanf(response.c_str(), "%f,%f,%d,%d,%d,%d",
                            &receivedData.temperature, &receivedData.humidity,
                            &receivedData.light, &receivedData.soil,
                            &receivedData.water, &receivedData.motor);
                            
  if (parsed == 6) {
    sensorData = receivedData;
    checkWateringCondition();
  } else {
    Serial.println("Loi: Du lieu I2C bi sai dinh dang!");
  }
}

void sendCommandToArduino(const char* command) {
  Wire.beginTransmission(ARDUINO_I2C_ADDR);
  Wire.write(command);
  Wire.endTransmission();
}