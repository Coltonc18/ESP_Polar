#include "Parameters.h"

BoundedQueue<uint16_t> ppiQueue(NUM_SAMPLES);
uint32_t PPI_Count = 0;
uint16_t prevMeasurement = 0;
float HRV_MeanPPI = 0.0;
float HRV_MedianPPI = 0.0;
uint16_t HRV_MaxPPI = 0;
uint16_t HRV_MinPPI = UINT16_MAX;
float M2 = 0.0;
float HRV_SDPPI = 0.0;
uint16_t HRV_Prc20PPI = 0;
uint16_t HRV_Prc80PPI = 0;
float sum2Diff = 0.0;
uint16_t HRV_RMSSD = 0;
uint32_t HRV_PPI50_Count = 0;
float HRV_pPPI50 = 0;
float HRV_HTI = 0;
uint16_t HRV_TIPPI = 0;
uint16_t hist[NUM_BINS] = { 0 };
uint8_t maxBinValue = 0;
MEM_Context mem_ctx;
float HRV_TotalPower = 0;
float HRV_LF = 0;
float HRV_HF = 0;
float HRV_LF_HF_Ratio = 0;

void resetHRVParameters(void) {
  ppiQueue.clear();
  PPI_Count = 0;
  prevMeasurement = 0;
  HRV_MeanPPI = 0.0;
  HRV_MedianPPI = 0.0;
  HRV_MaxPPI = 0;
  HRV_MinPPI = UINT16_MAX;
  M2 = 0.0;
  HRV_SDPPI = 0.0;
  HRV_Prc20PPI = 0;
  HRV_Prc80PPI = 0;
  sum2Diff = 0.0;
  HRV_RMSSD = 0;
  HRV_PPI50_Count = 0;
  HRV_pPPI50 = 0.0;
  HRV_HTI = 0.0;
  HRV_TIPPI = 0;
  for (int i = 0; i < NUM_BINS; ++i) {
    hist[i] = 0;
  }
  maxBinValue = 0;
  MEM_Init(&mem_ctx);
  HRV_TotalPower = 0;
  HRV_LF = 0;
  HRV_HF = 0;
  HRV_LF_HF_Ratio = 0;
}

void updateHRVParameters(uint16_t measurement) {
  uint16_t popped = ppiQueue.enqueue(measurement);
  PPI_Count = ppiQueue.size();
  updateHistogram(measurement, popped);
  updateHRV_MaxPPI(measurement, popped);
  updateHRV_MinPPI(measurement, popped);
  updateHRV_SDPPI_Mean(measurement, popped);
  updateHRV_MedianPPI(measurement);
  updateHRV_Prc20PPI(measurement);
  updateHRV_Prc80PPI(measurement);
  updateHRV_RMSSD(measurement, popped);
  updateHRV_pPPI50(measurement, popped);
  updateHRV_HTI(measurement);
  updateHRV_TIPPI(measurement);
  updateMEM_Parameters(measurement);
  prevMeasurement = measurement;
}

void updateHistogram(uint16_t measurement, uint16_t popped) {
  // Calculate bin index
  uint8_t bin = PPI_TO_BIN(measurement);
  
  // Increment the hist bin count
  hist[bin]++;

  // Decrement the hist bin count from the popped value
  if (popped != NULL) {
    uint8_t popped_bin = PPI_TO_BIN(popped);
    hist[popped_bin]--;

    // If we popped from the bin that had the max value, we need to recalculate maxBinValue
    if (hist[popped_bin] + 1 == maxBinValue) {
      maxBinValue = 0;
      for (int i = 0; i < NUM_BINS; ++i) {
        if (hist[i] > maxBinValue) {
          maxBinValue = hist[i];
        }
      }
    }
    return;
  } 
  
  // Update the maximum bin value if necessary
  if (hist[bin] > maxBinValue) {
    maxBinValue = hist[bin];
  }
}

void updateHRV_MedianPPI(uint16_t measurement) {
  // Calculate bin index
  uint8_t bin = PPI_TO_BIN(measurement);
  
  // Calculate target position for median determination
  uint32_t target = (PPI_Count - 1) / 2;
  
  // Find the median bin
  uint32_t cumulative = 0;
  uint8_t median_bin = 0;
  for (int i = 0; i < NUM_BINS; i++) {
    cumulative += hist[i];
    if (cumulative > target) {
      median_bin = i;
      break;
    }
  }
  
  // Calculate and update median value
  HRV_MedianPPI = BIN_START + ((float) median_bin + 0.5) * BIN_WIDTH;
}

void updateHRV_MaxPPI(uint16_t measurement, uint16_t popped) {
  // If the popped value was the max, reset the max
  if (abs(HRV_MaxPPI - popped) < BIN_WIDTH) {
    HRV_MaxPPI = 0;
    // Find the max PPI interval
    for (int i = NUM_BINS - 1; i >= 0; i--) {
      if (hist[i] > 0) {
        HRV_MaxPPI = BIN_START + ((float) i + 0.5) * BIN_WIDTH;
        break;
      }
    }
  } else if (measurement > HRV_MaxPPI) {
    // Update the maximum PPI interval if new measurement is larger
    HRV_MaxPPI = measurement;
  }
}

void updateHRV_MinPPI(uint16_t measurement, uint16_t popped) {
  // If the popped value was the min, reset the min
  if (abs(HRV_MinPPI - popped) < BIN_WIDTH) {
    HRV_MinPPI = UINT16_MAX;
    // Find the min PPI interval
    for (int i = 0; i < NUM_BINS; i++) {
      if (hist[i] > 0) {
        HRV_MinPPI = BIN_START + ((float) i + 0.5) * BIN_WIDTH;
        break;
      }
    }
  } else if (measurement < HRV_MinPPI) {
    // Update the minimum PPI interval if new measurement is smaller
    HRV_MinPPI = measurement;
  }
}

void updateHRV_SDPPI_Mean(uint16_t measurement, uint16_t popped) {
  if (popped != NULL) {
    // Remove the contribution of the popped value
    double delta_old = popped - HRV_MeanPPI;
    HRV_MeanPPI = (HRV_MeanPPI * PPI_Count - popped) / (PPI_Count - 1);
    M2 -= delta_old * (popped - HRV_MeanPPI);
  }

  // Update running mean:
  //    δ = xₙ − μₙ₋₁
  //    μₙ = μₙ₋₁ + δ / n
  double delta = measurement - HRV_MeanPPI;
  HRV_MeanPPI += delta / PPI_Count;

  // Update M2 (sum of squared deviations):
  //    δ₂ = xₙ − μₙ   (using new mean)
  //    M2ₙ = M2ₙ₋₁ + δ * δ₂
  double delta2 = measurement - HRV_MeanPPI;
  M2 += delta * delta2;

  // Compute population stddev:
  //    σ = √(M2 / n)
  HRV_SDPPI = sqrt(M2 / PPI_Count);
}

void updateHRV_Prc20PPI(uint16_t measurement) {
  // Calculate bin index
  uint8_t bin PPI_TO_BIN(measurement);

  // Compute the 20%-rank:
  //    rank_20 = ⌈ 0.2 * PPI_Count ⌉
  uint32_t rank20 = (0.2 * (float)PPI_Count) + 1;

  // Find the bin where the cumulative count ≥ rank20
  uint32_t cumulative = 0;
  uint8_t p20_bin = 0;
  for (int i = 0; i < NUM_BINS; ++i) {
    cumulative += hist[i];
    if (cumulative >= rank20) {
      p20_bin = i;
      break;
    }
  }

  // 20th‐percentile = center of that bin
  //    p20_ms = BIN_START + (p20_bin + 0.5) * BIN_WIDTH
  HRV_Prc20PPI = BIN_START + ((float)p20_bin + 0.5) * BIN_WIDTH;
}

void updateHRV_Prc80PPI(uint16_t measurement) {
  // Calculate bin index
  uint8_t bin = PPI_TO_BIN(measurement);

  // Compute the 80%-rank:
  //    rank_80 = ⌈ 0.8 * PPI_Count ⌉
  uint32_t rank80 = (0.8 * (float)PPI_Count) + 1;

  // Find the bin where the cumulative count ≥ rank80
  uint32_t cumulative = 0;
  uint8_t p80_bin = 0;
  for (int i = 0; i < NUM_BINS; ++i) {
    cumulative += hist[i];
    if (cumulative >= rank80) {
      p80_bin = i;
      break;
      }
  }

  // 80th‐percentile = center of that bin
  //    p80_ms = BIN_START + (p80_bin + 0.5) * BIN_WIDTH
  HRV_Prc80PPI = BIN_START + ((float)p80_bin + 0.5) * BIN_WIDTH;
}

void updateHRV_RMSSD(uint16_t measurement, uint16_t popped) {
  if (popped != NULL) {
    // Remove the contribution of the popped value's differences
    // We need to get the value before popped in the queue
    uint16_t before_popped = ppiQueue.peek();  // This gets the oldest value
    float diff_old = float(popped) - float(before_popped);
    sum2Diff -= diff_old * diff_old;
  }

  float diff = float(measurement) - float(prevMeasurement);
  sum2Diff += diff * diff;
  HRV_RMSSD = sqrt(sum2Diff / float(PPI_Count - 1));
}

void updateHRV_pPPI50(uint16_t measurement, uint16_t popped) {
  if (popped != NULL) {
    // Remove the contribution of the popped value
    uint16_t before_popped = ppiQueue.peek();  // This gets the oldest value
    if (abs(popped - before_popped) > 50) {
      HRV_PPI50_Count--;
    }
  }

  if (abs(measurement - prevMeasurement) > 50) {
    HRV_PPI50_Count++;
  }
  HRV_pPPI50 = ((float)HRV_PPI50_Count / (PPI_Count-1)) * 100;
}

void updateHRV_HTI(uint16_t measurement) {
  // HTI is already based on the current histogram state
  // which is updated by updateHistogram, so we just need to recalculate
  // TODO: determine which measurement to use here... using most recent for now
  HRV_HTI = (float) PPI_Count / hist[PPI_TO_BIN(measurement)];
}

void updateHRV_TIPPI(uint16_t measurement) {
  // TIPPI is already based on the current histogram state and maxBinValue
  // which are updated by updateHistogram, so we just need to recalculate
  HRV_TIPPI = (2.0 * PPI_Count * BIN_WIDTH) / float(maxBinValue);
  // Serial.printf("%u\n", HRV_TIPPI);
}

void updateMEM_Parameters(uint16_t measurement) {
  ProcessNewPPI(&mem_ctx, measurement);
  HRV_TotalPower = mem_ctx.total_power;
  HRV_LF = mem_ctx.LF;
  HRV_HF = mem_ctx.HF;
  HRV_LF_HF_Ratio = mem_ctx.LF_HF_Ratio;
}

void printHRVParameters(uint16_t current_PPI) {
  // Print start marker, timestamp and all parameters in CSV format with fixed width
  Serial.print("START,");  // Line start marker
  Serial.printf("%.2f,%u,%u,%.2f,%.2f,%u,%u,%.2f,%u,%u,%u,%.2f,%.2f,%u,%.0f,%.2f,%.2f,%.2f,END\r\n",
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
    HRV_TIPPI,          // TIPPI
    HRV_TotalPower,     // Total Power
    HRV_LF,             // LF
    HRV_HF,             // HF
    HRV_LF_HF_Ratio     // LF/HF Ratio
  );
  delay(20);  // Increased delay to ensure complete transmission
}
