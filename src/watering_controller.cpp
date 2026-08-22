#include <Arduino.h>
#include "app_state.h"
#include "i2c_service.h"
#include "mqtt_service.h"
static unsigned long pumpStartTime = 0; 
const int PUMP_FLOW_RATE_ML_PER_S = 22;
void turnPumpOn(const char* reason) {
  if (sensorData.water <= 5) {
    Serial.println("Action REJECTED: Cannot turn ON pump. Water tank is empty!");
    Serial.print("Original reason: ");
    Serial.println(reason);
    publishEvent("off", "Rejected: Water tank empty");
    return;
  }

  if (pumpState) return;
  
  pumpState = true;
  pumpStartTime = millis(); // Bắt đầu bấm giờ
  
  // Tính toán thời gian sẽ tưới (để log ra cho dễ theo dõi)
  unsigned long durationMs = (wateringConfig.waterAmount * 1000) / 22;
  
  Serial.println("Action: Pump turned ON");
  Serial.print("Reason: ");
  Serial.println(reason);
  Serial.printf("Target: %d ml -> Duration: %lu ms\n", wateringConfig.waterAmount, durationMs);
  
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
  const bool isOutOfWater = sensorData.water <= 5; 

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
    
    // Tính toán xem đã bơm đủ thời gian (đủ waterAmount) chưa?
    unsigned long requiredDurationMs = (wateringConfig.waterAmount * 1000) / 22;
    const bool targetAmountReached = (millis() - pumpStartTime) >= requiredDurationMs;

    const bool soilIsWetEnough = sensorData.soil > wateringConfig.soilOffAbove;
    const bool temperatureTooHigh = sensorData.temperature > wateringConfig.tempStop;
    const bool humidityTooHigh = sensorData.humidity > wateringConfig.humidityStop;
    const bool lightTooHigh = sensorData.light > wateringConfig.lightStop;
    
    Serial.printf("Target Amount Reached: %s\nSoil wet enough: %s\nTemperature too high: %s\nOut of water: %s\n",
                  targetAmountReached ? "YES" : "NO", soilIsWetEnough ? "YES" : "NO", 
                  temperatureTooHigh ? "YES" : "NO", isOutOfWater ? "YES" : "NO");
                  
    if (isOutOfWater) {
      turnPumpOff("EMERGENCY STOP: Water tank is empty!");
    } 
    else if (targetAmountReached) {
      turnPumpOff("Watering STOP: Reached target waterAmount");
    }
    else if (soilIsWetEnough || temperatureTooHigh || humidityTooHigh || lightTooHigh) {
      turnPumpOff("Watering STOP: Sensor condition reached");
    } else {
      Serial.println("-> Continue watering");
    }
  }
  Serial.println("-------------------------");
}