#include "BLEReceiveTask.h"

TaskHandle_t BLEReceiveTask::taskHandle = NULL;
PolarBLEConnection* BLEReceiveTask::connection = nullptr;

void BLEReceiveTask::start() {
  connection = new PolarBLEConnection();

  // Start the BLE scan for the device
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new PolarBLEConnection::MyAdvertisedDeviceCallbacks(DEVICE_NAME));
  pBLEScan->setInterval(2500);
  pBLEScan->setWindow(1000);
  pBLEScan->setActiveScan(true);

  Serial.printf("Starting scan for %s...\n", DEVICE_NAME);
  pBLEScan->start(5, false);

  // Initialize scanning state
  connection->doScan = true;

  // Start the BLE task on Core 0 (priority 1)
  xTaskCreatePinnedToCore(taskFunction, "BLE_Task", 4096, NULL, 1, &taskHandle, 0);
}

void BLEReceiveTask::stop() {
  if (taskHandle != NULL) {
    vTaskDelete(taskHandle);
    taskHandle = NULL;
  }
  if (connection != nullptr) {
    delete connection;
    connection = nullptr;
  }
}

void BLEReceiveTask::taskFunction(void* parameters) {
  unsigned long scanStartTime = millis();  // Initialize scanStartTime immediately
  bool isScanning = true;

  while (1) {
    if (connection->doConnect == true) {
      if (connection->ConnectToServer()) {
        Serial.println("Connected to Polar Sense!");
        // Stop trying to connect and scan
        connection->doConnect = false;
        connection->doScan = false;
        isScanning = false;

        // Blink the onboard LED green five times
        for (int i = 0; i < 5; i++) {
          neopixelWrite(ONBOARD_LED, 0, 20, 0);
          delay(200);
          neopixelWrite(ONBOARD_LED, 0, 0, 0);
          delay(100);
        }
      } else {
        Serial.printf("Failed to connect to %s. Rescanning...\n", DEVICE_NAME);

        // Blink the onboard LED red three times
        for (int i = 0; i < 3; i++) {
          neopixelWrite(ONBOARD_LED, 20, 0, 0);
          delay(333);
          neopixelWrite(ONBOARD_LED, 0, 0, 0);
          delay(167);
        }
        neopixelWrite(ONBOARD_LED, 20, 0, 0);

        connection->doScan = true;
        isScanning = true;
        scanStartTime = millis();
      }
    } else if (isScanning) {
      // Check if we've been scanning for more than 10 seconds
      if (millis() - scanStartTime > 10000) {
        Serial.printf("%s not found within 10 seconds. Restarting scan...\n", DEVICE_NAME);

        // Blink the onboard LED yellow three times
        for (int i = 0; i < 3; i++) {
          neopixelWrite(ONBOARD_LED, 20, 20, 0);
          delay(333);
          neopixelWrite(ONBOARD_LED, 0, 0, 0);
          delay(167);
        }
        neopixelWrite(ONBOARD_LED, 20, 0, 0);

        BLEScan* pBLEScan = BLEDevice::getScan();
        pBLEScan->start(5, false);
        scanStartTime = millis();
      }
    }

    // Otherwise, do nothing!
    // Notifications will handle the rest of the work
    vTaskDelay(10 / portTICK_PERIOD_MS);  // give the CPU a break

    // Check to see if there is serial input
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n');
      if (input == "quit") {
        uint8_t endPpi[] = { 0x03, 0x03 };
        uint8_t endSdk[] = { 0x03, 0x09 };
        Serial.println("Ending PPI Measurements");
        connection->pControlCharacteristic->writeValue(endPpi, sizeof(endPpi), true);
        delay(500);
        Serial.println("Ending SDK Mode");
        connection->pControlCharacteristic->writeValue(endSdk, sizeof(endSdk), true);
        delay(500);
        Serial.println("Exiting...");
        ComputeTask::stop();
        BLEReceiveTask::stop();
        exit(0);
      }
    }
  }
}
