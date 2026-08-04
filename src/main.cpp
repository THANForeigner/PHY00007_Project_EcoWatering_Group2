#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseESP32.h>
#include <PubSubClient.h>
#include "keys.h"

const char* ssid = SECRET_WIFI_SSID;
const char* password = SECRET_WIFI_PASS;

// 1. Cleaned up hostname (No http://, no ports, no /?fbclid=...)
const char* mqtt_server = SECRET_MQTT_SERVER;
const int mqtt_port = SECRET_MQTT_PORT;

// 2. You MUST create these in your HiveMQ Cloud Access Management console
const char* mqtt_username = SECRET_MQTT_USER; 
const char* mqtt_password = SECRET_MQTT_PASS;

FirebaseConfig config;
FirebaseAuth auth;
FirebaseData firebaseData;

// 3. Use WiFiClientSecure for port 8883
WiFiClientSecure espClient;
PubSubClient client(espClient);

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

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    // Attempt to connect using the username and password
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Connected to HiveMQ!");
      // client.subscribe("your/topic/here"); // Subscribe to a topic here
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  config.database_url = SECRET_FIREBASE_HOST;
  config.signer.tokens.legacy_token = SECRET_FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  espClient.setInsecure(); 
  client.setServer(mqtt_server, mqtt_port);
}

void send_to_firebase()
{
  int temperature = random(20, 35);
  int humidity = random(40, 80);

  Serial.print("Sending Data - Temp: ");
  Serial.print(temperature);
  Serial.print(" Hum: ");
  Serial.println(humidity);

  // 4. Write data to Firebase
  // The path "/sensor/temperature" creates those nodes in the database if they don't exist
  if (Firebase.setInt(firebaseData, "/sensor/temperature", temperature)) {
    Serial.println("Temperature saved successfully.");
  } else {
    Serial.println("Failed to save temperature.");
    Serial.print("Reason: ");
    Serial.println(firebaseData.errorReason());
  }

  if (Firebase.setInt(firebaseData, "/sensor/humidity", humidity)) {
    Serial.println("Humidity saved successfully.\n");
  } else {
    Serial.println("Failed to save humidity.");
    Serial.print("Reason: ");
    Serial.println(firebaseData.errorReason());
  }
  delay(5000);
}
void receive_from_mqtt()
{

}
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  send_to_firebase();
}