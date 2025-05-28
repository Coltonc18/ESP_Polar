#ifndef _CONSTANTS_H
#define _CONSTANTS_H

#include <Arduino.h>
#include <esp32-hal-ledc.h>

#define NULL 0

#define DEVICE_NAME "Polar Sense"

// Pinout definitions
#define ONBOARD_LED 48    // ESP32-S3 has an onboard Neopixel attached to this pin
#define PWM_PIN  21       // Attatched to GPIO pin 21
#define PWM_FREQ 5000     // 5 kHz
#define PWM_RES  12       // 12-bit resolution

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// MEM-based PSD Estimation Parameters
#define MODEL_ORDER ((int)(NUM_SAMPLES / log(2*NUM_SAMPLES) + 0.5))
#define FREQ_START 0.04   // Start frequency of the PSD
#define FREQ_END 0.4      // End frequency of the PSD
#define FREQ_BINS 50      // Number of frequency bins

// Constants for parameters
#define NUM_SAMPLES 30    // Number of samples to store to compute moving averages
#define NUM_BINS 218      // Number of bins in the histogram (300 - 2000 ms) / 7.8125 ms
#define BIN_WIDTH 7.815   // Width of histogram bins (in ms)
#define BIN_START 300.0   // Lowest PPI value in the histogram (in ms)
#define BIN_END 2000.0    // Highest PPI value in the histogram (in ms)
#define HIST_WIDTH BIN_END - BIN_START  // Width of the histogram (in ms)
#define MAX_PPI_DIFF 300  // Maximum difference between consecutive PPI samples to be considered valid

#define PPI_QUEUE_SIZE 15 // Maximum number of PPI samples to store in the receive queue

// Function macro to convert PPI to bin index
#define PPI_TO_BIN(x) \
    (((int)(((x) - BIN_START) / BIN_WIDTH) < 0) ? 0 : \
    (((int)(((x) - BIN_START) / BIN_WIDTH) >= NUM_BINS) ? (NUM_BINS-1) : \
    (int)(((x) - BIN_START) / BIN_WIDTH)))

#endif  // _CONSTANTS_H 