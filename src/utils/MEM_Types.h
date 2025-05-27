#ifndef MEM_TYPES_H
#define MEM_TYPES_H

#include "Constants.h"

// Structure to hold MEM algorithm context
typedef struct {
  int index;                    // Current write position
  uint32_t samples_processed;   // Number of samples processed
  float buffer[NUM_SAMPLES];    // Circular buffer
  float ar_coeff[MODEL_ORDER];  // Autoregressive coefficients
  float psd[FREQ_BINS];         // Power spectrum
} MEM_Context;

#endif // MEM_TYPES_H
