#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

int LRD_PIN = A0;
int SMS_PIN = A1;

#define DHT_PIN A2
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

int TRIG_PIN = 2;
int ECHO_PIN = 3;
int BUTTON_PIN = 4;
int MOTOR_PIN = 7;

float currentTemp = 0.0;
float currentHum = 0.0;
int currentLight = 0;
int currentSoil = 0;
int currentWaterLevel = 0;
int currentButton = 0;
bool motorState = false;

volatile bool espJustReadData = false; 
bool needLcdUpdate = false;
unsigned long lastEspSyncTime = 0;

volatile bool commandReceived = false;
volatile char commandBuffer[20];
char i2cData[40] = "0.0,0.0,0,0,0,0";

unsigned long buttonPressTime = 0;
bool isButtonPressed = false;
bool actionExecuted = false;

unsigned long previousLcdMillis = 0;
const long displayInterval = 5000;
int currentScreen = 0;

const int SOIL_RAW_DRY = 1023;
const int SOIL_RAW_WET = 0;
const int LIGHT_RAW_DARK = 0;
const int LIGHT_RAW_BRIGHT = 1023;


// ==========================================
// HÀM ĐIỀU KHIỂN BƠM (RELAY ACTIVE LOW)
// ==========================================
void setMotor(bool state) {
  motorState = state;
  digitalWrite(MOTOR_PIN, motorState ? HIGH : LOW);
  
  Serial.print("Motor -> ");
  Serial.println(motorState ? "ON" : "OFF");
}


// ==========================================
// CÁC HÀM NGẮT I2C
// ==========================================
void receiveCommandFromESP(int howMany) {
  int index = 0;
  while (Wire.available() && index < sizeof(commandBuffer) - 1) {
    commandBuffer[index++] = Wire.read();
  }
  commandBuffer[index] = '\0';
  commandReceived = true;
}

void sendDataToESP() {
  Wire.write((const uint8_t*)i2cData, strlen(i2cData));
  espJustReadData = true;
}


// ==========================================
// CÁC HÀM HỖ TRỢ
// ==========================================
long getDistance() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;
  return duration * 0.034 / 2;
}

int mapPercent(int raw, int in_min, int in_max) {
  int percent = map(raw, in_min, in_max, 0, 100);
  return constrain(percent, 0, 100);
}


// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(9600);
  Serial.println("\nHe thong da khoi dong!");

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("Smart Watering");
  lcd.setCursor(0, 1); lcd.print("Initializing...");

  pinMode(LRD_PIN, INPUT);
  pinMode(SMS_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP); 
  
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);
  motorState = false;

  dht.begin();

  Wire.begin(8); 
  Wire.setWireTimeout(30000, true); 
  Wire.onReceive(receiveCommandFromESP);
  Wire.onRequest(sendDataToESP);

  delay(2000);
  lcd.clear();
}


// ==========================================
// VÒNG LẶP CHÍNH
// ==========================================
void loop() {
  unsigned long currentMillis = millis();

  // ==========================================
  // BƯỚC 1: XỬ LÝ NÚT BẤM (NHẤN GIỮ 2 GIÂY)
  // ==========================================
  int rawButton = digitalRead(BUTTON_PIN);
  currentButton = (rawButton == LOW) ? 1 : 0; 

  if (currentButton == 1) { 
    if (!isButtonPressed) {
      isButtonPressed = true;
      buttonPressTime = currentMillis;
      actionExecuted = false;
    } else {
      if (!actionExecuted && (currentMillis - buttonPressTime >= 2000)) {
        setMotor(!motorState); 
        actionExecuted = true; 
      }
    }
  } else {
    isButtonPressed = false;
    actionExecuted = false;
  }

  // ==========================================
  // BƯỚC 2: XỬ LÝ LỆNH TỪ ESP QUA I2C
  // ==========================================
  if (commandReceived) {
    char cmdCopy[20];
    noInterrupts(); 
    strncpy(cmdCopy, (const char*)commandBuffer, sizeof(cmdCopy) - 1);
    cmdCopy[sizeof(cmdCopy) - 1] = '\0';
    commandReceived = false;
    interrupts();
    
    Serial.println(cmdCopy);
    
    if (strstr(cmdCopy, "PUMP_ON") != NULL) {
      setMotor(true);
    } 
    else if (strstr(cmdCopy, "PUMP_OFF") != NULL) {
      setMotor(false);
    }
  }

  // ==========================================
  // BƯỚC 3: THU THẬP DỮ LIỆU CẢM BIẾN
  // ==========================================
  currentLight = mapPercent(analogRead(LRD_PIN), LIGHT_RAW_DARK, LIGHT_RAW_BRIGHT);
  currentSoil = mapPercent(analogRead(SMS_PIN), SOIL_RAW_DRY, SOIL_RAW_WET);
  
  long dist = getDistance();
  if (dist >= 0) currentWaterLevel = constrain(map(dist, 9, 0, 0, 100), 0, 100);
  else currentWaterLevel = 0;

  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) currentTemp = t;
  if (!isnan(h)) currentHum = h;

  // ==========================================
  // BƯỚC 4: CHUẨN BỊ DỮ LIỆU ĐỂ ESP LẤY
  // ==========================================
  char tempStr[8], humStr[8];
  dtostrf(currentTemp, 4, 1, tempStr); 
  dtostrf(currentHum, 4, 1, humStr);

  char tempBuffer[40];
  snprintf(tempBuffer, sizeof(tempBuffer), "%s,%s,%d,%d,%d,%d",
           tempStr, humStr, 100 - currentLight, currentSoil, currentWaterLevel, motorState ? 1 : 0);

  noInterrupts();
  strncpy(i2cData, tempBuffer, sizeof(i2cData) - 1);
  i2cData[sizeof(i2cData) - 1] = '\0';
  interrupts();

  // ==========================================
  // BƯỚC 5: BIỂU DIỄN LÊN LCD (5s / Lần)
  // ==========================================
  if (currentMillis - previousLcdMillis >= displayInterval) {
    needLcdUpdate = true;
    previousLcdMillis = currentMillis; 
  }

  if (currentMillis - lastEspSyncTime > 10000) {
    espJustReadData = true; 
  }

  if (needLcdUpdate && espJustReadData) {
    delay(5); 
    lastEspSyncTime = millis(); 

    lcd.clear();
    switch (currentScreen) {
      case 0:
        lcd.setCursor(0, 0); lcd.print("Light: "); lcd.print(currentLight); lcd.print("%");
        lcd.setCursor(0, 1); lcd.print("LDR Sensor");
        break;
      case 1:
        lcd.setCursor(0, 0); lcd.print("Temp: "); lcd.print(currentTemp, 1); lcd.print(" C");
        lcd.setCursor(0, 1); lcd.print("DHT11");
        break;
      case 2:
        lcd.setCursor(0, 0); lcd.print("Hum: "); lcd.print(currentHum, 1); lcd.print(" %");
        lcd.setCursor(0, 1); lcd.print("DHT11");
        break;
      case 3:
        lcd.setCursor(0, 0); lcd.print("Soil: "); lcd.print(currentSoil); lcd.print(" %");
        lcd.setCursor(0, 1); lcd.print("Moisture");
        break;
      case 4:
        lcd.setCursor(0, 0); lcd.print("Water: "); lcd.print(currentWaterLevel); lcd.print("%");
        lcd.setCursor(0, 1); lcd.print("Tank Level");
        break;
      case 5:
        lcd.setCursor(0, 0); lcd.print("Motor: "); lcd.print(motorState ? "ON" : "OFF");
        lcd.setCursor(0, 1); lcd.print("Button: "); lcd.print(currentButton);
        break;
    }
    
    currentScreen++;
    if (currentScreen > 5) currentScreen = 0; 

    needLcdUpdate = false;
    espJustReadData = false; 
  }
}