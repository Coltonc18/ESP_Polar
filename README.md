# ESP32 Polar HRV Monitor

A real-time heart rate variability (HRV) monitoring system built on the ESP32-S3 platform that interfaces with Polar heart rate sensors. This project provides real-time HRV analysis and PWM output control for biofeedback applications.

## Quick Links

- [Setup Guide](docs/setup.md) - Installation and configuration instructions
- [API Documentation](docs/API.md) - Technical documentation
- [Dependencies](docs/dependencies.md) - Required libraries and components

## Key Features

- **BLE Connectivity**: Seamless connection with Polar heart rate sensors
- **Real-time HRV Analysis**: Tracks a multitude of HRV parameters
- **PWM Output Control**: 12-bit resolution PWM output for biofeedback applications
- **Multithreaded Architecture**: Separate cores for BLE communication and HRV processing
- **Data Validation**: PPI difference validation to ensure data quality
- **CSV Output**: Real-time data logging in CSV format

## Getting Started

1. See the [Setup Guide](docs/setup.md) for installation instructions
2. Review the [API Documentation](docs/API.md) for technical details
3. Check [Dependencies](docs/dependencies.md) for required components

## License

Copyright ©2025 AMP Lab, University of Washington. All rights reserved.
Authored by Colton Carroll.

## Acknowledgments

- AMP Lab, University of Washington
- Polar for their heart rate sensor technology
