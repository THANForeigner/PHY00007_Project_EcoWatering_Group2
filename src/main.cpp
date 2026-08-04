#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> // Required for building and parsing JSON payloads
#include "keys.h"

const char* ssid = SECRET_WIFI_SSID;
const char* password = SECRET_WIFI_PASS;

const char* mqtt_server = SECRET_MQTT_SERVER;
const int mqtt_port = SECRET_MQTT_PORT;
const char* mqtt_username = SECRET_MQTT_USER; 
const char* mqtt_password = SECRET_MQTT_PASS;

WiFiClientSecure espClient;
PubSubClient client(espClient);

// Device state variables
bool pumpState = false;
unsigned long lastStatusTime = 0;
const unsigned long STATUS_INTERVAL = 5000; // Publish status every 5 seconds

int envThreshold[9] = {35, 65, 18, 32, 35, 70, 85, 30, 80};

int wateringAmount = 300;//amount (ml)

void setup_wifi() {
  delay(10);
  Serial.println("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

// Handle incoming MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(msg);

  if (String(topic) == "watering/cmd") {
    msg.toLowerCase();
    if (msg == "on" || msg == "1" || msg == "true" || msg == "high" || msg == "open") {
      pumpState = true;
      Serial.println("Action: Pump turned ON");
      // take actions
    } 
    else if (msg == "off" || msg == "0" || msg == "false" || msg == "low" || msg == "closed") {
      pumpState = false;
      Serial.println("Action: Pump turned OFF");
      // take actions
    }
  }

  else if (String(topic) == "watering/config") {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, msg);
    if (error) {
      Serial.print("Failed to parse config JSON: ");
      Serial.println(error.c_str());
      return; 
    }
    if (doc["type"] == "watering") {
      Serial.println("Valid watering config received. Updating thresholds...");
      // read 
      envThreshold[0] = doc["soilOnBelow"] | 35;
      envThreshold[1] = doc["soilOffAbove"] | 65;
      envThreshold[2] = doc["tempOnMinC"] | 18;
      envThreshold[3] = doc["tempOnMaxC"] | 32;
      envThreshold[4] = doc["soilOnBelow"] | 35;
      envThreshold[5] = doc["soilOffAbove"] | 70;
      envThreshold[6] = doc["tempOnMinC"] | 85;
      envThreshold[7] = doc["tempOnMaxC"] | 30;
      envThreshold[8] = doc["tempOnMinC"] | 80;
      wateringAmount = doc["waterAmount"] | 300;
      Serial.printf("New Thresholds\n: ");
      for(int i = 0; i<9; i++)
      {
        Serial.printf("%d ",envThreshold[i]);
      }
      Serial.println();
      // take actions
    } else {
      Serial.println("Warning: Config type is not 'watering'");
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    String clientId = "ESP32Client-esp-01-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT Broker!");
      
      // Subscribe to the command and config topics specified in README
      client.subscribe("watering/cmd");
      client.subscribe("watering/config");
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Publish telemetry to watering/status
void publish_status() {
  StaticJsonDocument<256> doc;
  
  // Create JSON payload matching the README specs
  doc["state"] = pumpState ? "on" : "off";
  doc["temp"] = random(200, 350) / 10.0; // Simulated data (e.g., 24.5)
  doc["hum"] = random(40, 80);           // Simulated data
  doc["soil"] = random(20, 60);          // Simulated data
  doc["light"] = random(10, 90);         // Simulated data
  doc["rssi"] = WiFi.RSSI();
  
  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);
  
  Serial.print("Publishing Status: ");
  Serial.println(jsonBuffer);
  client.publish("watering/status", jsonBuffer);
}

// Publish strict event object to watering/event
void publish_event(const char* action, const char* reason) {
  StaticJsonDocument<256> doc;
  
  doc["type"] = "watering";
  doc["action"] = action;
  doc["reason"] = reason;
  doc["soil"] = random(20, 60);
  doc["temp"] = random(200, 350) / 10.0;
  doc["hum"] = random(40, 80);
  doc["light"] = random(10, 90);
  
  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);
  
  client.publish("watering/event", jsonBuffer);
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  
  espClient.setInsecure(); // Skip SSL certificate validation for simplicity
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); // Attach the incoming message handler
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Must be called regularly to process incoming MQTT messages

  // Non-blocking status publish (replaces blocking delay() calls)
  unsigned long currentMillis = millis();
  if (currentMillis - lastStatusTime >= STATUS_INTERVAL) {
    lastStatusTime = currentMillis;
    publish_status();
  }
}