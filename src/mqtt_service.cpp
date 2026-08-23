#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

#include "app_state.h"
#include "config.h"
#include "watering_controller.h"
extern bool requestManualPumpOn;
extern bool requestManualPumpOff;
namespace {
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

void applyWateringConfig(JsonDocument& doc) {
  wateringConfig.soilOnBelow = doc["soilOnBelow"] | 35;
  wateringConfig.soilOffAbove = doc["soilOffAbove"] | 65;
  wateringConfig.tempMin = doc["tempMin"] | 18;
  wateringConfig.tempMaxWatering = doc["tempMaxWatering"] | 32;
  wateringConfig.tempStop = doc["tempStop"] | 35;
  wateringConfig.humidityMaxWatering = doc["humidityMaxWatering"] | 70;
  wateringConfig.humidityStop = doc["humidityStop"] | 85;
  wateringConfig.lightMaxWatering = doc["lightMaxWatering"] | 30;
  wateringConfig.lightStop = doc["lightStop"] | 80;
  wateringConfig.waterAmount = doc["waterAmount"] | 300;

  Serial.println("Valid watering config received.");
  Serial.printf("Soil: %d - %d %%\n", wateringConfig.soilOnBelow, wateringConfig.soilOffAbove);
  Serial.printf("Temperature: %d - %d C, Stop: %d C\n", wateringConfig.tempMin,
                wateringConfig.tempMaxWatering, wateringConfig.tempStop);
  Serial.printf("Humidity: < %d%%, Stop: > %d%%\n", wateringConfig.humidityMaxWatering,
                wateringConfig.humidityStop);
  Serial.printf("Light: < %d%%, Stop: > %d%%\n", wateringConfig.lightMaxWatering,
                wateringConfig.lightStop);
  Serial.printf("Water amount: %d ml\n", wateringConfig.waterAmount);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; ++i) message += static_cast<char>(payload[i]);

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  if (String(topic) == MQTT_COMMAND_TOPIC) {
    message.toLowerCase();
    if (message == "on" || message == "1" || message == "true" || message == "high" || message == "open") {
      requestManualPumpOn = true;
      requestManualPumpOff = false;
      Serial.println("MQTT Request: Bật bơm (Đang chờ hàm checkWateringCondition xử lý...)");
    } else if (message == "off" || message == "0" || message == "false" || message == "low" || message == "closed") {
      requestManualPumpOff = true;
      requestManualPumpOn = false;
      Serial.println("MQTT Request: Tắt bơm (Đang chờ hàm checkWateringCondition xử lý...)");
    }
    return;
  }

  if (String(topic) == MQTT_CONFIG_TOPIC) {
    StaticJsonDocument<768> doc;
    const DeserializationError error = deserializeJson(doc, message);
    if (error) {
      Serial.print("Failed to parse config JSON: ");
      Serial.println(error.c_str());
    } else if (doc["type"] == "watering") {
      applyWateringConfig(doc);
    } else {
      Serial.println("Warning: Config type is not 'watering'");
    }
  }
}
}  // namespace

void setupWifi() {
  delay(10);
  Serial.printf("\nConnecting to WiFi: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setupMqtt() {
  espClient.setInsecure();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(1024);
}

void maintainMqttConnection() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("Connected to MQTT Broker!");
      mqttClient.subscribe(MQTT_COMMAND_TOPIC);
      mqttClient.subscribe(MQTT_CONFIG_TOPIC);
    } else {
      Serial.printf("failed, rc=%d try again in 5 seconds\n", mqttClient.state());
      delay(5000);
    }
  }
}

void mqttLoop() { mqttClient.loop(); }

void publishStatus() {
  StaticJsonDocument<768> doc;
  doc["state"] = pumpState ? "on" : "off";
  doc["temp"] = sensorData.temperature;
  doc["hum"] = sensorData.humidity;
  doc["soil"] = sensorData.soil;
  doc["light"] = sensorData.light;
  doc["water"] = sensorData.water;
  doc["rssi"] = WiFi.RSSI();
  
  // JsonObject thresholds = doc["thresholds"].to<JsonObject>();
  // thresholds["soilOnBelow"] = wateringConfig.soilOnBelow;
  // thresholds["soilOffAbove"] = wateringConfig.soilOffAbove;
  // thresholds["tempMin"] = wateringConfig.tempMin;
  // thresholds["tempMaxWatering"] = wateringConfig.tempMaxWatering;
  // thresholds["tempStop"] = wateringConfig.tempStop;
  // thresholds["humidityMaxWatering"] = wateringConfig.humidityMaxWatering;
  // thresholds["humidityStop"] = wateringConfig.humidityStop;
  // thresholds["lightMaxWatering"] = wateringConfig.lightMaxWatering;
  // thresholds["lightStop"] = wateringConfig.lightStop;
  // thresholds["waterAmount"] = wateringConfig.waterAmount;

  char jsonBuffer[768];
  serializeJson(doc, jsonBuffer);
  Serial.print("Publishing Status: ");
  Serial.println(jsonBuffer);
  mqttClient.publish(MQTT_STATUS_TOPIC, jsonBuffer);
}

void publishEvent(const char* action, const char* reason) {
  StaticJsonDocument<512> doc;
  doc["type"] = "watering";
  doc["action"] = action;
  doc["reason"] = reason;
  doc["soil"] = sensorData.soil;
  doc["temp"] = sensorData.temperature;
  doc["hum"] = sensorData.humidity;
  doc["light"] = sensorData.light;

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  mqttClient.publish(MQTT_EVENT_TOPIC, jsonBuffer);
}
