#include "BLEReceiveTask.h"
#include "esp32-hal-ledc.h"

TaskHandle_t BLEReceiveTask::taskHandle = NULL;
PolarBLEConnection* BLEReceiveTask::connection = nullptr;

void BLEReceiveTask::start() {
  connection = new PolarBLEConnection();

  // Start the BLE scan for the device
  Serial.println("Starting scan...");
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new PolarBLEConnection::MyAdvertisedDeviceCallbacks("Polar Sense"));
  pBLEScan->setInterval(2500);
  pBLEScan->setWindow(1000);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);

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
  while (1) {
    if (connection->doConnect == true) {
      if (connection->ConnectToServer()) {
        Serial.println("Connected to Polar Sense!");
        // Stop trying to connect
        connection->doConnect = false;

        // Blink the onboard LED green three times
        for (int i = 0; i < 3; i++) {
          neopixelWrite(ONBOARD_LED, 0, 20, 0);
          delay(333);
          neopixelWrite(ONBOARD_LED, 0, 0, 0);
          delay(167);
        }
      }
      else {
        Serial.println("Failed to connect to the requested device. Rescanning...");
        connection->doScan = true;
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
        vTaskDelete(NULL);
        exit(0);
      }
    }
  }
}
