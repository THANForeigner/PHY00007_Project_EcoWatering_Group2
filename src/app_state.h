#pragma once

struct SensorData {
  float temperature = 0.0F;
  float humidity = 0.0F;
  int light = 0;
  int soil = 0;
  int water = 0;
  int button = 0;
};

struct WateringConfig {
  int soilOnBelow = 35;
  int soilOffAbove = 65;
  int tempMin = 18;
  int tempMaxWatering = 32;
  int tempStop = 35;
  int humidityMaxWatering = 70;
  int humidityStop = 85;
  int lightMaxWatering = 30;
  int lightStop = 80;
  int waterAmount = 300;
};

extern SensorData sensorData;
extern WateringConfig wateringConfig;
extern bool pumpState;
