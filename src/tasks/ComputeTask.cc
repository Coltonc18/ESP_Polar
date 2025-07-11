#include "ComputeTask.h"

TaskHandle_t ComputeTask::taskHandle = NULL;

void ComputeTask::start() {
  // Setup PWM on Channel 0 (Pin 21)
  ledcAttach(PWM_PIN, PWM_FREQ, PWM_RES);  // Attach gpio to channel 0

  // Start the PWM task on Core 1 (priority 2, higher than BLE)
  xTaskCreatePinnedToCore(taskFunction, "PWM_Task", 4096, NULL, 2, &taskHandle, 1);
}

void ComputeTask::stop() {
  if (taskHandle != NULL) {
    vTaskDelete(taskHandle);
    taskHandle = NULL;
  }
}

void ComputeTask::taskFunction(void* parameters) {
  PPIData currentData;
  uint16_t prevPPI = 0;
  uint16_t validPPI = 0;
  uint32_t lastProcessTime = 0;
  float dutyCycle;
  bool valid = false;

  while (1) {
    if (xQueueReceive(PolarBLEConnection::ppiQueue, &currentData, portMAX_DELAY) == pdTRUE) {
      // Update voltage output if currentData is valid and not too different from the last measurement
      valid = ((abs(currentData.ppi - prevPPI) < MAX_PPI_DIFF) || prevPPI < BIN_START) && currentData.valid;

      // Store the most recent valid PPI
      validPPI = valid ? currentData.ppi : prevPPI;

      // If the newest PPI was valid, update the previous PPI, otherwise keep the previous PPI
      prevPPI = valid ? currentData.ppi : prevPPI;

      // Calculate duty cycle
      dutyCycle = ((float)4095 / HIST_WIDTH) * (validPPI - BIN_START);

      // Write voltage output to PWM_PIN
      ledcWrite(PWM_PIN, (uint32_t)dutyCycle);

      // Update all HRV parameters given the previous PPI measurement
      if (validPPI > 0)
        updateHRVParameters(validPPI);

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
