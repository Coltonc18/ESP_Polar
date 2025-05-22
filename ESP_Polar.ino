#include "PolarBLEConnection.h"
#include "Parameters.h"
#include "BoundedQueue.hpp"
#include "esp32-hal-ledc.h"
#include "src/tasks/BLEReceiveTask.h"
#include "src/tasks/ComputeTask.h"

#define ONBOARD_LED 48    // ESP32-S3 has an onboard Neopixel attached to this pin

void setup() {
  Serial.begin(115200);
  delay(500);
  neopixelWrite(ONBOARD_LED, 10, 0, 0);

  // Start the BLE and Compute tasks
  BLEReceiveTask::start();
  ComputeTask::start();
}

void loop() {
  // Empty loop, tasks are handled in the background
}
