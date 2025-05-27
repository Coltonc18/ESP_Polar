#ifndef BLERECEIVE_TASK_H
#define BLERECEIVE_TASK_H

#include "ComputeTask.h"
// ComputeTask includes all other dependencies

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
