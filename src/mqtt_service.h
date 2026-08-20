#pragma once

void setupWifi();
void setupMqtt();
void maintainMqttConnection();
void mqttLoop();
void publishStatus();
void publishEvent(const char* action, const char* reason);
