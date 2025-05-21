/*
 * Copyright ©2025 AMP Lab, University of Washington. All rights reserved.
 * Authored by Colton Carroll.
 */

#include "PolarBLEConnection.h"
#include "Parameters.h"
#include "BoundedQueue.hpp"
#include "esp32-hal-ledc.h"

#define ONBOARD_LED 48    // ESP32-S3 has an onboard Neopixel attached to this pin
#define MAX_PPI_DIFF 300  // Max allowable difference between two consecutive PPI measurements
#define PWM_PIN  21       // Attatched to GPIO pin 21
#define PWM_FREQ 5000     // 5 kHz
#define PWM_RES  12       // 12-bit resolution

PolarBLEConnection* connection;
TaskHandle_t pwmTask = NULL;

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("Starting BLE Client application...");
  neopixelWrite(ONBOARD_LED, 10, 0, 0);

  // Print CSV header
  // Serial.println("START,Timestamp,PPI_Count,Mean_PPI,Median_PPI,Max_PPI,SD_PPI,Prc20_PPI,Prc80_PPI,RMSSD,pPPI50,HTI,TIPPI,END");

  // Setup PWM on Channel 0 (Pin 21)
  ledcAttach(PWM_PIN, PWM_FREQ, PWM_RES);  // Attach gpio to channel 0
  
  // Start a BLE connection to the Polar Device
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
  xTaskCreatePinnedToCore(BLE_Task, "BLE_Task", 4096, NULL, 1, NULL, 0);

  // Start the PWM task on Core 1 (priority 2, higher than BLE)
  xTaskCreatePinnedToCore(PWM_Task, "PWM_Task", 4096, NULL, 2, &pwmTask, 0);
}

void loop() { }  // Empty loop, BLE tasks are handled in the background

void BLE_Task(void *pvParameters) {
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
      } else {
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
        uint8_t endPpi[] =  {0x03, 0x03};
        uint8_t endSdk[] =  {0x03, 0x09};
        Serial.println("Ending PPI Measurements");
        connection->pControlCharacteristic->writeValue(endPpi, sizeof(endPpi), true);
        delay(500);
        Serial.println("Ending SDK Mode");
        connection->pControlCharacteristic->writeValue(endSdk, sizeof(endSdk), true);
        delay(500);
        Serial.println("Exiting...");
        vTaskDelete(pwmTask);
        vTaskDelete(NULL);
        exit(0);
      }
    }
  }
}

void PWM_Task(void *pvParameters) {
  PPIData currentData;
  uint16_t prevPPI = 0;
  uint16_t validPPI = 0;
  uint32_t lastProcessTime = 0;
  float dutyCycle;
  bool valid = false;
  
  while (1) {
    if (xQueueReceive(PolarBLEConnection::ppiQueue, &currentData, portMAX_DELAY) == pdTRUE) {
      // Update voltage output if currentData is valid and not too different from the last measurement
      valid = (((currentData.ppi - prevPPI) < MAX_PPI_DIFF) || prevPPI == 0) && currentData.valid;
      // Serial.printf("Valid? %d (newest ppi diff was %d and valid bit was %d\n", valid, currentData.ppi-prevPPI, currentData.valid);
      // vTaskDelay(20);

      // Store the most recent valid PPI
      validPPI = valid ? currentData.ppi : prevPPI;

      // If the newest PPI was valid, update the previous PPI, otherwise keep the previous PPI
      prevPPI = valid ? currentData.ppi : prevPPI;
      // Serial.printf("\tValid PPI : %u, prevPPI : %u\n", validPPI, prevPPI);
      
      // Calculate duty cycle
      dutyCycle = ((float) 4095 / HIST_WIDTH) * (validPPI - BIN_START);

      // Write voltage output to PWM_PIN
      ledcWrite(PWM_PIN, (uint32_t) dutyCycle);
      
      // Update all HRV parameters given the previous PPI measurement
      if (validPPI > 0)
        updateHRVParameters(validPPI);

      // Serial.println(String(valid ? "VALID" : "INVALID") +
      //                 "\tPPI: " + String(currentData.ppi) +
      //                 "\tInferred HR: " + String(60000 / currentData.ppi) + " bpm" +
      //                 "\tDuty Cycle: " + String(dutyCycle) +
      //                 "\tVoltage: " + String(dutyCycle * 3.3 / 4095) + "V");
      printHRVParameters(validPPI);
      
      // Dynamic delay management
      uint32_t processingTime = millis() - lastProcessTime;
      uint32_t requiredDelay = validPPI > processingTime ? 
                               validPPI - processingTime : 0;

      vTaskDelay(pdMS_TO_TICKS(requiredDelay));
      lastProcessTime = millis();
    }
  }
}

void printHRVParameters(uint16_t current_PPI) {
  // Print start marker, timestamp and all parameters in CSV format with fixed width
  Serial.print("START,");  // Line start marker
  Serial.printf("%.2f,%u,%u,%.2f,%.2f,%u,%u,%.2f,%u,%u,%u,%.2f,%.2f,%u,END\r\n",
    millis() / 1000.0,  // Timestamp (seconds since start)
    PPI_Count,          // PPI Count
    current_PPI,        // Most recent PPI measurement
    HRV_MeanPPI,        // Mean PPI
    HRV_MedianPPI,      // Median PPI
    HRV_MinPPI,         // Min PPI
    HRV_MaxPPI,         // Max PPI
    HRV_SDPPI,          // SD PPI
    HRV_Prc20PPI,       // 20th Percentile PPI
    HRV_Prc80PPI,       // 80th Percentile PPI
    HRV_RMSSD,          // RMSSD
    HRV_pPPI50,         // pPPI50
    HRV_HTI,            // HTI
    HRV_TIPPI           // TIPPI
  );
  delay(20);       // Increased delay to ensure complete transmission
}
