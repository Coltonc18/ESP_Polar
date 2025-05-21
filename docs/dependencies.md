# Dependencies

## Arduino IDE

Download and install the Arduino IDE from the official website:
[https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)

### Library Dependencies

- FreeRTOS
  - Required for task management and queues
  - Included with ESP32 Arduino core
- BLE Libraries
  - BLEDevice.h
  - BLEUtils.h
  - BLEScan.h
  - BLEAdvertisedDevice.h
  - Included with ESP32 Arduino core
- esp32-hal-ledc.h
  - PWM control for ESP32 boards
  - Included with ESP32 Arduino core
- Standard C++ Libraries
  - queue
  - cstdint
  - Included with Arduino IDE

### Board Dependencies

- Developed for use on the ESP32-S3
- Arduino IDE Setup:
  1. Install ESP32 board support through Arduino Board Manager
     - Add URL: <https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json>
     - Install "ESP32 by Espressif Systems"
  2. Select "ESP32S3 Dev Module" from board menu
  3. Set USB CDC On Boot to "Enabled"
  4. Set USB DFU On Boot to "Enabled"
  5. Set Upload Mode to "UART0 / Hardware CDC"
  6. Set CPU Frequency to "240MHz (WiFi/BT)"
  7. Set Flash Mode to "QIO"
  8. Set Flash Size to "4MB (32Mb)"
  9. Set Partition Scheme to "Default 4MB with spiffs"
  10. Set Core Debug Level to "None"
  11. Set PSRAM to "Enabled"
  12. Upload code to board
