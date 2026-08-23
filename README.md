
The Arduino is responsible for directly interfacing with the sensors (DHT11, LDR, Soil Moisture, Ultrasonic) and the water pump relay [cite: 1]. 

If you want to extract the Arduino code (`Arduino_code/Arduino_code.ino`) to run it independently on a physical Arduino board, keep the following in mind:

### Hardware Wiring Requirements
When deploying the standalone Arduino code, wire your components according to the defined pins [cite: 1]:
*   **LDR Sensor:** `A0`
*   **Soil Moisture Sensor:** `A1`
*   **DHT11 Sensor:** `A2`
*   **Ultrasonic Sensor:** TRIG to `2`, ECHO to `3`
*   **Manual Button:** `4` (Uses `INPUT_PULLUP`)
*   **Water Pump Relay (Motor):** `7`
*   **LCD I2C Display:** Connect to Arduino SDA/SCL (Address `0x27`) [cite: 1].

### Library Dependencies
You must install the following libraries in your Arduino IDE to compile the `.ino` file [cite: 1]:
*   `LiquidCrystal_I2C`
*   `DHT sensor library`

### I2C Master/Slave Behavior
*   **Current State:** The Arduino is configured as an I2C Slave at address `8` (`Wire.begin(8)`) [cite: 1]. It listens for commands (`PUMP_ON`/`PUMP_OFF`) from the ESP8266 and sends sensor data payloads back when requested [cite: 1].
*   **Running Without ESP8266:** If the ESP8266 is disconnected, the Arduino will still run its `loop()` [cite: 1]. It will continue to read sensors every cycle, update the LCD every 5 seconds, and allow manual motor toggling via the physical button on Pin 4 [cite: 1]. 
*   **Standalone Automation:** In the current architecture, the *automation logic* (thresholds for temperature, soil moisture, etc.) resides on the ESP8266 (`src/watering_controller.cpp`) [cite: 1]. If you want the Arduino to automatically trigger the pump without the ESP8266, you must copy the threshold logic into the Arduino's `loop()` to evaluate `currentSoil`, `currentTemp`, etc., and call `setMotor(true/false)` accordingly [cite: 1].
