#ifndef _PARAMETERS_H
#define _PARAMETERS_H

#include "../utils/Constants.h"
#include "../utils/BoundedQueue.hpp"
#include "./MEM.h"

// Queue to store PPI measurements
extern BoundedQueue<uint16_t> ppiQueue;

// Number of PPI measurements received so far
extern uint32_t PPI_Count;

// Previous PPI measurement
extern uint16_t prevMeasurement;

// Mean PPI Interval
extern float HRV_MeanPPI;

// Median PPI Interval (Middle value of sorted PPI intervals)
extern float HRV_MedianPPI;

// Maximum PPI Interval
extern uint16_t HRV_MaxPPI;

// Minimum PPI Interval
extern uint16_t HRV_MinPPI;

// Variable used in Welford's algorithm for calculating aggregate variance (sum of squared deviations)
extern float M2;

// Standard Deviation of PPI Intervals (√[ Σ (PPIᵢ - MeanPPI)² / (N - 1) ])
extern float HRV_SDPPI;

// 20th Percentile of PPI Intervals (Value below which 20% of sorted PPI intervals fall)
extern uint16_t HRV_Prc20PPI;

// 80th Percentile of PPI Intervals (Value below which 80% of sorted PPI intervals fall)
extern uint16_t HRV_Prc80PPI;

// Sum of squared differences between adjacent PPI intervals
extern float sum2Diff;

// Root Mean Square of Successive Differences 
// (Square root of the mean of squared differences between adjacent PPI intervals: √[ Σ (PPIᵢ₊₁ - PPIᵢ)² / (N - 1) ])
extern uint16_t HRV_RMSSD;

// Percentage of Differences > 50 ms
// (Percentage of adjacent PPI intervals differing by > 50 ms: (Count(PPIᵢ₊₁ - PPIᵢ> 50ms) / (N - 1)) * 100)
extern uint32_t HRV_PPI50_Count;
extern float HRV_pPPI50;

// HRV Triangular Index
// Total number of PPI intervals divided by the height of the modal bin in the PPI histogram (standard bin width 7.8125 ms): N / Y
// where N is the total number of PPI intervals and Y is the height of the modal bin
extern float HRV_HTI;

// Triangular Interpolation of PPI Histogram
// Baseline width of the PPI interval histogram determined by triangular interpolation: M - N
extern uint16_t HRV_TIPPI;

// Histogram of PPI intervals
// Bin size is 7.8125 ms. Allowable range of PPI intervals is 300 - 2000 ms
extern uint16_t hist[NUM_BINS];
extern uint8_t maxBinValue;

// MEM-based PSD Estimation Context
extern MEM_Context mem_ctx;

// Low Frequency Power
extern float HRV_LF;

// High Frequency Power
extern float HRV_HF;

// Ratio of Low Frequency Power to High Frequency Power
extern float HRV_LF_HF_Ratio;

// Function prototypes
void resetHRVParameters(void);  // Reset all HRV parameters to default values
void updateHRVParameters(uint16_t measurement);  // Update all HRV parameters at once
void printHRVParameters(uint16_t measurement);   // Print all HRV parameters

// Methods to update each HRV parameter individually
void updateHRV_MedianPPI(uint16_t measurement);
void updateHRV_MaxPPI(uint16_t measurement, uint16_t popped);
void updateHRV_MinPPI(uint16_t measurement, uint16_t popped);
void updateHRV_SDPPI_Mean(uint16_t measurement, uint16_t popped);
void updateHRV_Prc20PPI(uint16_t measurement);
void updateHRV_Prc80PPI(uint16_t measurement);
void updateHRV_RMSSD(uint16_t measurement, uint16_t popped);
void updateHRV_pPPI50(uint16_t measurement, uint16_t popped);
void updateHRV_HTI(uint16_t measurement);
void updateHRV_TIPPI(uint16_t measurement);
void updateHistogram(uint16_t measurement, uint16_t popped);

#endif  // _PARAMETERS_H
