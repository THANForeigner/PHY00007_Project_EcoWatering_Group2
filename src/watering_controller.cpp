#include <Arduino.h>

#include "app_state.h"
#include "i2c_service.h"
#include "mqtt_service.h"

void turnPumpOn(const char* reason) {
  if (pumpState) return;
  pumpState = true;
  Serial.println("Action: Pump turned ON");
  Serial.print("Reason: ");
  Serial.println(reason);
  sendCommandToArduino("PUMP_ON");
  publishEvent("on", reason);
}

void turnPumpOff(const char* reason) {
  if (!pumpState) return;
  pumpState = false;
  Serial.println("Action: Pump turned OFF");
  Serial.print("Reason: ");
  Serial.println(reason);
  sendCommandToArduino("PUMP_OFF");
  publishEvent("off", reason);
}

void checkWateringCondition() {
  if (isnan(sensorData.temperature) || isnan(sensorData.humidity)) {
    Serial.println("-> Khong the kiem tra dieu kien tuoi!");
    return;
  }
  Serial.printf("Soil: %d%%\nTemperature: %.1f C\nHumidity: %.1f%%\nLight: %d%%\nWater Level: %d%%\nPump: %s\n",
                sensorData.soil, sensorData.temperature, sensorData.humidity,
                sensorData.light, sensorData.water, pumpState ? "ON" : "OFF");
  const bool isOutOfWater = sensorData.water < 10; 

  if (!pumpState) { // TRẠNG THÁI: BƠM ĐANG TẮT
    const bool soilIsDry = sensorData.soil < wateringConfig.soilOnBelow;
    const bool temperatureIsSuitable = sensorData.temperature >= wateringConfig.tempMin &&
                                       sensorData.temperature <= wateringConfig.tempMaxWatering;
    const bool humidityIsSuitable = sensorData.humidity < wateringConfig.humidityMaxWatering;
    const bool lightIsSuitable = sensorData.light < wateringConfig.lightMaxWatering;
    Serial.printf("Soil dry: %s\nTemperature suitable: %s\nHumidity suitable: %s\nLight suitable: %s\nHas Water: %s\n",
                  soilIsDry ? "YES" : "NO", temperatureIsSuitable ? "YES" : "NO",
                  humidityIsSuitable ? "YES" : "NO", lightIsSuitable ? "YES" : "NO",
                  !isOutOfWater ? "YES" : "NO");
    if (soilIsDry && temperatureIsSuitable && humidityIsSuitable && lightIsSuitable && !isOutOfWater) {
      turnPumpOn("Soil dry and all watering conditions satisfied");
    } else {
      if (isOutOfWater && soilIsDry) {
        Serial.println("-> No watering (Warning: Water tank is empty!)");
      } else {
        Serial.println("-> No watering");
      }
    }
  } else { // TRẠNG THÁI: BƠM ĐANG BẬT (ĐANG TƯỚI)
    const bool soilIsWetEnough = sensorData.soil > wateringConfig.soilOffAbove;
    const bool temperatureTooHigh = sensorData.temperature > wateringConfig.tempStop;
    const bool humidityTooHigh = sensorData.humidity > wateringConfig.humidityStop;
    const bool lightTooHigh = sensorData.light > wateringConfig.lightStop;
    Serial.printf("Soil wet enough: %s\nTemperature too high: %s\nHumidity too high: %s\nLight too high: %s\nOut of water: %s\n",
                  soilIsWetEnough ? "YES" : "NO", temperatureTooHigh ? "YES" : "NO",
                  humidityTooHigh ? "YES" : "NO", lightTooHigh ? "YES" : "NO", 
                  isOutOfWater ? "YES" : "NO");
    if (isOutOfWater) {
      turnPumpOff("EMERGENCY STOP: Water tank is empty!");
    } 
    else if (soilIsWetEnough || temperatureTooHigh || humidityTooHigh || lightTooHigh) {
      turnPumpOff("Watering stop condition reached");
    } else {
      Serial.println("-> Continue watering");
    }
  }
  Serial.println("-------------------------");
}