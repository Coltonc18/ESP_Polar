#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>
#include <Arduino.h>

#define NULL 0

#define ONBOARD_LED 48    // ESP32-S3 has an onboard Neopixel attached to this pin

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#endif  // CONFIG_H
