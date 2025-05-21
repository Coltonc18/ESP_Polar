# Setup Guide

This guide provides detailed instructions for setting up and configuring the ESP32 Polar HRV Monitor project.

## Prerequisites

### Hardware Requirements

- ESP32-S3 Development Board
- Polar Sense heart rate sensor
- USB cable for programming and serial communication
- Optional: External circuit for PWM output (connected to GPIO 21)

### Software Requirements

- Arduino IDE (version 2.0 or later)
- ESP32 board support package
- Required Libraries:
  - ESP32 BLE Arduino
  - ESP32 HAL LEDC
  - Custom libraries (included in the project):
    - PolarBLEConnection
    - Parameters
    - BoundedQueue

## Installation Steps

1. **Install Arduino IDE**
   - Download and install [Arduino IDE](https://www.arduino.cc/en/software)
   - Launch Arduino IDE

2. **Add ESP32 Board Support**
   - Open Arduino IDE
   - Go to File > Preferences
   - Add the following URL to "Additional Boards Manager URLs": <https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json>
   - Go to Tools > Board > Boards Manager
   - Search for "esp32"
   - Install "ESP32 by Espressif Systems"

3. **Install Required Libraries**
   - Go to Tools > Manage Libraries
   - Search for and install:
     - "ESP32 BLE Arduino"
     - "ESP32 HAL LEDC"

4. **Clone the Repository**

   ```bash
   git clone https://github.com/Coltonc18/ESP_Polar.git
   cd ESP_Polar
   ```

5. **Open the Project**
   - Open Arduino IDE
   - Go to File > Open
   - Navigate to the cloned repository
   - Open `ESP_Polar.ino`

## Configuration

### Board Configuration

1. Select the correct board:
   - Go to Tools > Board > ESP32 Arduino
   - Select "ESP32S3 Dev Module"

2. Configure the port:
   - Go to Tools > Port
   - Select the COM port where your ESP32-S3 is connected

### Project Configuration

The following parameters can be modified in the code:

#### BLE Configuration

```cpp
#define MAX_PPI_DIFF 300  // Maximum allowed difference between consecutive PPI measurements
```

#### PWM Configuration

```cpp
#define PWM_PIN  21       // GPIO pin for PWM output
#define PWM_FREQ 5000     // PWM frequency in Hz
#define PWM_RES  12       // PWM resolution in bits
```

## Testing the Setup

1. **Upload the Code**
   - Click the Upload button (→) in Arduino IDE
   - Wait for the upload to complete

2. **Open Serial Monitor**
   - Go to Tools > Serial Monitor
   - Set baud rate to 115200
   - You should see "Starting BLE Client application..."

3. **Connect Polar Sensor**
   - Power on your Polar Sense sensor
   - The ESP32 will automatically scan for and connect to the sensor
   - The onboard LED will blink green three times when connected

4. **Verify Data Output**
   - Check the Serial Monitor for CSV data output
   - Verify PWM output on GPIO 21 if connected to external circuit

## Troubleshooting

### Common Issues

1. **Board Not Found**
   - Ensure ESP32 board support is properly installed
   - Try reinstalling the ESP32 board package
   - Check USB cable and port

2. **Upload Failures**
   - Press and hold the BOOT button on ESP32-S3 during upload
   - Check if the correct board is selected
   - Verify the correct COM port is selected

3. **BLE Connection Issues**
   - Ensure Polar sensor is powered on
   - Check if the sensor is in pairing mode
   - Verify the sensor's battery level

4. **Serial Monitor Issues**
   - Verify baud rate is set to 115200
   - Check if the correct COM port is selected
   - Try closing and reopening the Serial Monitor

## Next Steps

After completing the setup:

1. Review the [API Documentation](API.md) for detailed information about the code structure
2. Check the [README](../README.md) for project overview and features
3. Start collecting and analyzing HRV data
