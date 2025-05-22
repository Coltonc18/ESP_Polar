#ifndef BLERECEIVE_TASK_H
#define BLERECEIVE_TASK_H

#include "PolarBLEConnection.h"
#include "esp32-hal-ledc.h"

class BLEReceiveTask {
public:
  static void start();
  static void stop();

private:
  static void taskFunction(void* parameters);
  static TaskHandle_t taskHandle;
  static PolarBLEConnection* connection;
};

#endif // BLERECEIVE_TASK_H
