# API Documentation

## Core Classes

### PolarBLEConnection

The `PolarBLEConnection` class manages the Bluetooth Low Energy (BLE) connection with Polar heart rate sensors.

#### Key Methods

- `ConnectToServer()`: Establishes connection with the Polar sensor
- `MyAdvertisedDeviceCallbacks`: Callback class for handling BLE device discovery
- `ppiQueue`: Queue for storing PPI (Peak-to-Peak Interval) data

#### Usage Example

```cpp
PolarBLEConnection* connection = new PolarBLEConnection();
BLEScan* pBLEScan = BLEDevice::getScan();
pBLEScan->setAdvertisedDeviceCallbacks(
    new PolarBLEConnection::MyAdvertisedDeviceCallbacks("Polar Sense")
);
```

### Parameters

The `Parameters` class manages Heart Rate Variability (HRV) calculations and data storage.

#### Key Parameters

- `HRV_MeanPPI`: Mean Peak-to-Peak Interval
- `HRV_MedianPPI`: Median Peak-to-Peak Interval
- `HRV_RMSSD`: Root Mean Square of Successive Differences
- `HRV_pPPI50`: Percentage of PPI differences > 50ms
- `HRV_HTI`: Heart Turbulence Index
- `HRV_TIPPI`: Time Index of PPI

#### Configuration Constants

For detailed configuration instructions, see the [Setup Guide](setup.md).

## Tasks

### BLETask

The BLE task runs on Core 0 with priority 1 and handles all BLE communication.

#### Key Responsibilities

- Device scanning and connection
- Data reception from Polar sensor
- Connection state management
- Command processing (e.g., 'quit' command)

#### Implementation Details

```cpp
void BLE_Task(void *pvParameters) {
    // Task runs continuously
    // Handles connection and data reception
    // Processes serial commands
}
```

### PWMTask

The PWM task runs on Core 1 with priority 2 and handles HRV processing and PWM output.

#### Key Responsibilities

- PPI data validation
- HRV parameter calculation
- PWM output control
- Data logging

#### Implementation Details

```cpp
void PWM_Task(void *pvParameters) {
    // Processes PPI data from queue
    // Validates measurements
    // Updates HRV parameters
    // Controls PWM output
    // Logs data in CSV format
}
```

#### PWM Configuration

For detailed PWM configuration instructions, see the [Setup Guide](setup.md).

## Data Format

### CSV Output Structure

```txt
Timestamp,PPI_Count,Mean_PPI,Median_PPI,Max_PPI,SD_PPI,Prc20_PPI,Prc80_PPI,RMSSD,pPPI50,HTI,TIPPI
```

### PPI Data Structure

```cpp
struct PPIData {
    uint16_t ppi;    // Peak-to-Peak Interval in milliseconds
    bool valid;      // Validity flag for the measurement
};
```

## Error Handling

### PPI Validation

- Measurements are considered valid if:
  - The difference from the previous PPI is less than `MAX_PPI_DIFF`
  - The measurement's validity flag is true
- Invalid measurements are replaced with the last valid PPI

### Connection Management

- Automatic reconnection attempts on connection failure
- LED feedback for connection status
- Graceful shutdown on 'quit' command

For troubleshooting common issues, see the [Setup Guide](setup.md).
