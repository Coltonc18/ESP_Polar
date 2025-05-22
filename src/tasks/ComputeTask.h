#ifndef COMPUTE_TASK_H
#define COMPUTE_TASK_H

#include "../utils/Config.h"
#include "../core/PolarBLEConnection.h"
#include "../core/Parameters.h"

#include <esp32-hal-ledc.h>

#define PWM_PIN  21       // Attatched to GPIO pin 21
#define PWM_FREQ 5000     // 5 kHz
#define PWM_RES  12       // 12-bit resolution

class ComputeTask {
public:
  static void start();
  static void stop();

private:
  static void taskFunction(void* parameters);
  static TaskHandle_t taskHandle;
};

#endif // COMPUTE_TASK_H
