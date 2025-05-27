#ifndef COMPUTE_TASK_H
#define COMPUTE_TASK_H

#include "../core/PolarBLEConnection.h"
#include "../core/Parameters.h"

class ComputeTask {
public:
  static void start();
  static void stop();

private:
  static void taskFunction(void* parameters);
  static TaskHandle_t taskHandle;
};

#endif // COMPUTE_TASK_H
