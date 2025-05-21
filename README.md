# ESP32 Polar HRV Monitor

A real-time heart rate variability (HRV) monitoring system built on the ESP32-S3 platform that interfaces with Polar heart rate sensors. This project provides real-time HRV analysis and PWM output control for biofeedback applications.

## Features

- **BLE Connectivity**: Seamless connection with Polar heart rate sensors
- **Real-time HRV Analysis**: Calculates multiple HRV parameters including:
  - Mean and Median PPI
  - RMSSD (Root Mean Square of Successive Differences)
  - pPPI50 (Percentage of PPI differences > 50ms)
  - HTI (Heart Turbulence Index)
  - TIPPI (Time Index of PPI)
- **PWM Output Control**: 12-bit resolution PWM output for biofeedback applications
- **Multithreaded Architecture**:
  - Core 0: BLE communication (Priority 1)
  - Core 1: PWM control and HRV processing (Priority 2)
- **Data Validation**: Implements PPI difference validation to ensure data quality
- **CSV Output**: Real-time data logging in CSV format for analysis

## Quick Start

For detailed setup and installation instructions, please refer to the [Setup Guide](docs/setup.md).

## Data Output Format

The system outputs data in CSV format with the following columns:

- Timestamp (seconds)
- PPI Count
- Current PPI
- Mean PPI
- Median PPI
- Min/Max PPI
- SD PPI
- 20th/80th Percentile PPI
- RMSSD
- pPPI50
- HTI
- TIPPI

## Documentation

- [Setup Guide](docs/setup.md) - Detailed installation and configuration instructions
- [API Documentation](docs/API.md) - Technical documentation of the code structure and components
- [Project Dependencies](docs/dependencies.md) - Dependencies of the project specific to the ESP32 and Arduino IDE

## Hardware Requirements

- ESP32-S3 Development Board
- Polar Sense heart rate sensor
- USB cable for programming and serial communication
- Optional: External RC circuit for PWM output (connected to GPIO 21)

## Software Requirements

- Arduino IDE with ESP32 board support
- Required Libraries:
  - ESP32 BLE Arduino
  - ESP32 HAL LEDC
  - Custom libraries (included in the project):
    - PolarBLEConnection
    - Parameters
    - BoundedQueue

## Installation

1. Clone this repository:

   ```bash
   git clone https://github.com/Coltonc18/ESP_Polar.git
   ```

2. Open the project in Arduino IDE
3. Install required libraries and boards through Arduino IDE (see [Dependencies](docs/dependencies.md))
4. Select your ESP32-S3 board in the Arduino IDE
5. Upload the code to your ESP32-S3

## Usage

1. Power on your ESP32-S3 and Polar Sense sensor
2. The ESP32 will automatically scan for and connect to the Polar sensor
3. Once connected, the onboard LED will blink green three times
4. HRV data will be output via Serial in CSV format
5. PWM output will be available on GPIO 21
6. Type 'quit' in the Serial monitor to end the session

## Configuration

Key parameters can be modified in the code:

- `MAX_PPI_DIFF`: Maximum allowed difference between consecutive PPI measurements
- `PWM_FREQ`: PWM frequency (default: 5000Hz)
- `PWM_RES`: PWM resolution (default: 12-bit)

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

Copyright ©2025 AMP Lab, University of Washington. All rights reserved.
Authored by Colton Carroll.

## Acknowledgments

- AMP Lab, University of Washington
- Polar for their heart rate sensor technology
