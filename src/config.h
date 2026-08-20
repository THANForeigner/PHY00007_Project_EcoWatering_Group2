#pragma once

#include "keys.h"

constexpr int ARDUINO_I2C_ADDR = 8;
constexpr unsigned long STATUS_INTERVAL = 5000;
constexpr const char* WIFI_SSID = SECRET_WIFI_SSID;
constexpr const char* WIFI_PASSWORD = SECRET_WIFI_PASS;
constexpr const char* MQTT_SERVER = SECRET_MQTT_SERVER;
constexpr int MQTT_PORT = SECRET_MQTT_PORT;
constexpr const char* MQTT_USERNAME = SECRET_MQTT_USER;
constexpr const char* MQTT_PASSWORD = SECRET_MQTT_PASS;
constexpr const char* MQTT_COMMAND_TOPIC = "watering/cmd";
constexpr const char* MQTT_CONFIG_TOPIC = "watering/config";
constexpr const char* MQTT_STATUS_TOPIC = "watering/status";
constexpr const char* MQTT_EVENT_TOPIC = "watering/event";
