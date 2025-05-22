#include "src/core/PolarBLEConnection.h"
#include "src/core/Parameters.h"
#include "src/utils/BoundedQueue.hpp"
#include "src/utils/Config.h"
#include "src/tasks/BLEReceiveTask.h"
#include "src/tasks/ComputeTask.h"

#include <esp32-hal-ledc.h>

void setup() {
  Serial.begin(115200);
  delay(500);
  neopixelWrite(ONBOARD_LED, 10, 0, 0);

  // Start the BLE and Compute tasks
  BLEReceiveTask::start();
  ComputeTask::start();
}

void loop() {
  // Empty loop, tasks are handled in the background by notifications
}
