#include "PolarBLEConnection.h"

void processPpgData(uint64_t time, float ppgGrn, float ppgRed, float ppgInf, float ppgAmb);

boolean PolarBLEConnection::connected = false;
boolean PolarBLEConnection::doConnect = false;
boolean PolarBLEConnection::doScan = false;

BLERemoteCharacteristic* PolarBLEConnection::pControlCharacteristic = nullptr;
BLERemoteCharacteristic* PolarBLEConnection::pDataCharacteristic = nullptr;
BLEAdvertisedDevice* PolarBLEConnection::myDevice = nullptr;

BLEUUID PolarBLEConnection::serviceUUID;
BLEUUID PolarBLEConnection::controlCharUUID;
BLEUUID PolarBLEConnection::dataCharUUID;

QueueHandle_t PolarBLEConnection::ppiQueue;

PolarBLEConnection::PolarBLEConnection() :
  PolarBLEConnection(
    SERVICE_UUID,
    CONTROL_CHAR_UUID,
    DATA_CHAR_UUID) {
}

PolarBLEConnection::PolarBLEConnection(String serviceUUID,
  String controlCharUUID,
  String dataCharUUID) {
  this->serviceUUID = BLEUUID(serviceUUID.c_str());
  this->controlCharUUID = BLEUUID(controlCharUUID.c_str());
  this->dataCharUUID = BLEUUID(dataCharUUID.c_str());

  this->ppiQueue = xQueueCreate(PPI_QUEUE_SIZE, sizeof(PPIData));
}

// Call the correct callback function based on the type of measurement
void PolarBLEConnection::NotifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {

  switch (pData[0] & 0x3F) {
  case 0x01:
    PpgNotifyCallback(pData, length);
    break;
  case 0x03:
    PpiNotifyCallback(pData, length);
    break;
  default:
    String response = "Response: [";
    if (length > 0) {
      for (int i = 0; i < length; i++) {
        response += String(pData[i], HEX) + " ";
      }
      response += "]";
      Serial.println(response);
    }
  }  // end switch
}

// Parses the PPG data
void PolarBLEConnection::PpgNotifyCallback(
  uint8_t*& pData,
  size_t& length) {

  // Extract the timestamp from bytes 1 - 8
  uint64_t timestamp = 0;
  for (int i = 0; i < 8; i++) {
    timestamp |= (uint64_t)pData[i + 1] << (8 * i);
  }

  // Check if the PPG measurement is of type 0x00
  if (pData[9] & 0x7F != 0x00) {
    Serial.printf("PPG Measurement is of type %d\n", pData[9] & 0x7F);
    return;
  }

  if (pData[0] & 0x80 != 0) {  // Data is compressed
    const uint8_t CHANNELS = 0x04;
    // Resolution is 0x16 (22 bit value)
    const uint8_t RESOLUTION = 0x16;
    // This takes up 3 bytes (almost)
    const uint8_t RES_BYTES = 0x03;

    // Calculate Reference Sample (first sample)
    uint8_t refSampleIndex = 10;  // = 10
    uint8_t refSampleSize = CHANNELS * RES_BYTES;  // = 12

    uint32_t maskPmdData = -0x1 << RESOLUTION;
    uint32_t bitmask = maskPmdData - 1;

    // For each of the reference values (one for each channel)
    std::vector<int32_t> refSample(CHANNELS);
    for (int i = 0; i < CHANNELS; i++) {
      // Construct a 24 bit int from the three bytes of data for each channel
      int32_t value = 0;
      for (int j = 0; j < 3; j++) {
        value |= (pData[refSampleIndex + i * 3 + j] << (j * 8));
      }

      // If the result is negative, apply a bitmask to sign-extend it to 32 bits
      if ((value & bitmask) < 0) {
        value |= 0xFFFFFFFF << (3 * 8);
      }

      // Save reference sample into array
      refSample[i] = value;
    }

    // Now that we have a reference sample, we can calculate the rest based off the delta frames

    // Find the Delta Frame Size
    uint8_t frameSizeIndex = (CHANNELS * RES_BYTES) + 10;  // = 22
    uint8_t frameSize = pData[frameSizeIndex];  // In bits
    // Samples are tightly packed (compressed) and will span byte boundaries.
    // Number of bits per sample = frameSize*CHANNELS.
    // Bytes per sample = ceil(bits per sample / 8.0)

    // Find the Sample Count
    uint8_t sampleCountIndex = frameSizeIndex + 1;  // = 23
    uint8_t sampleCount = pData[sampleCountIndex];

    // Extract the samples from the packet following the reference sample
    std::vector<std::vector<int32_t>> samples(sampleCount + 1, std::vector<int32_t>(CHANNELS));
    samples[0] = refSample;
    uint8_t deltaDataIndex = sampleCountIndex + 1;  // = 24
    uint8_t bitIndex = 0;

    // Extract the samples from the packet following the reference sample
    for (int sample = 1; sample <= sampleCount; sample++) {
      for (int channel = 0; channel < CHANNELS; channel++) {
        int32_t delta = 0;
        // Extract the delta value from the compressed data
        for (int bit = 0; bit < frameSize; bit++) {
          int byteIndex = deltaDataIndex + ((bitIndex + bit) / 8);
          int bitOffset = (bitIndex + bit) % 8;
          int bitValue = (pData[byteIndex] >> bitOffset) & 0x01;
          delta = ((delta | bitValue) << 1);
        }
        bitIndex += frameSize;

        // If the delta is negative, sign extend it to 32 bits
        if (delta != 0) {
          delta |= (INT32_MAX << (frameSize - 1));
        }

        // Add the delta to the previous sample to get the current sample
        samples[sample][channel] = samples[sample - 1][channel] + delta;
      }
    }

    // Implement later when I find a conversion factor for the PPG values...
    // const float CONVERSION_FACTOR = CONVERSION_FACTOR_FROM_SETTINGS;
    // // Maybe std::transform would be better here?
    // for (auto& sample : samples) {
    //   for (auto& value : sample) {
    //     value *= CONVERSION_FACTOR;
    //   }
    // }

    // Print out the PPG data
    for (const auto& sample : samples) {
      float ppgGrn = sample[0];
      float ppgRed = sample[1];
      float ppgInf = sample[2];
      float ppgAmb = sample[3];

      processPpgData(timestamp, ppgGrn, ppgRed, ppgInf, ppgAmb);
    }

  } else {
    Serial.println("Data packet is uncompressed. Parsing not implemented.");
  }
}

// Print out the ppg measurement values in a CSV format
void processPpgData(uint64_t time, float ppgGrn, float ppgRed, float ppgInf, float ppgAmb) {
  if ((ppgGrn != 0) && (ppgRed != 0) && (ppgInf != 0) && (ppgAmb != 0)) {
    // CSV output (Add header to top of file)
    Serial.printf("%"PRIu64",%8.0f,%8.0f,%8.0f,%8.0f\n", time, ppgGrn, ppgRed, ppgInf, ppgAmb);
  }
}

// Parses the PPI data
void PolarBLEConnection::PpiNotifyCallback(
  uint8_t*& pData,
  size_t& length) {

  for (int i = 10; i < length; i += 6) {
    // Extract the data from the current ppi response
    uint8_t heartRate = pData[i];
    uint16_t ppi = pData[i + 1] | (pData[i + 2] << 8);
    uint16_t ppError = pData[i + 3] | (pData[i + 4] << 8);
    uint8_t flags = pData[i + 5];

    PPIData data;
    data.timestamp = millis();
    data.heartRate = heartRate;
    data.ppi = ppi;
    data.ppError = ppError;
    data.flags = flags;
    data.valid = !(flags & 0x1) && ppError > 0 && ppError < 30;  // ignore skin flags for now

    xQueueSendToBack(ppiQueue, &data, 0);
  }
}

void PolarBLEConnection::ReadData() {
  if (pControlCharacteristic->canRead()) {
    String rawData = pControlCharacteristic->readValue();
    String response = "Control says: ";
    for (int i = 0; i < rawData.length(); i++) {
      response += String(rawData[i], HEX) + " ";
    }
    Serial.println(response);
  } else {
    Serial.println(" * Couldn't read from control characteristic");
  }
}

bool PolarBLEConnection::ConnectToServer() {
  Serial.print("Connecting to ");
  Serial.println(myDevice->getAddress().toString());

  BLEClient* pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // neopixelWrite(ONBOARD_LED, 0, 0, 0);

  // Connect to the remote BLE Server.
  pClient->connect(myDevice);
  Serial.println(" - Connected to server");

  // Set MTU size
  if (pClient->setMTU(MTU)) {
    Serial.printf(" - MTU set to %d\n", MTU);
  } else {
    Serial.println(" * Failed to set MTU.");
    return false;
  }

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.println(" * Failed to find our service UUID.");
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our expected service UUID");

  // Obtain references to the characteristics in the service of the remote BLE server.
  pControlCharacteristic = pRemoteService->getCharacteristic(controlCharUUID);
  pDataCharacteristic = pRemoteService->getCharacteristic(dataCharUUID);
  if (pControlCharacteristic == nullptr || pDataCharacteristic == nullptr) {
    Serial.println("Failed to find our characteristic UUID");
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our control and data characteristic UUIDs");

  // Register for notifications
  if (pDataCharacteristic->canNotify()) {
    pDataCharacteristic->registerForNotify([this](BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
      this->NotifyCallback(pBLERemoteCharacteristic, pData, length, isNotify);
      });
    Serial.println(" - Registered for data notifications");
  } else {
    Serial.println(" * Data characteristic doesn't support notifications");
  }

  if (pControlCharacteristic->canNotify()) {
    pControlCharacteristic->registerForNotify([this](BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
      this->NotifyCallback(pBLERemoteCharacteristic, pData, length, isNotify);
      });
    Serial.println(" - Registered for control notifications");
  } else if (pControlCharacteristic->canIndicate()) {
    pControlCharacteristic->registerForNotify([this](BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
      this->NotifyCallback(pBLERemoteCharacteristic, pData, length, isNotify);
      }, false);
    Serial.println(" - Registered for control indications");
  } else {
    Serial.println(" * Control characteristic doesn't support notifications or indications");
  }

  uint8_t getPpg[] = { 0x01, 0x01 };
  uint8_t getAccel[] = { 0x01, 0x02 };
  uint8_t getPpi[] = { 0x01, 0x03 };

  uint8_t startPpg[] = { 0x02, 0x01, 0x00, 0x01, 0x87, 0x00, 0x01, 0x01, 0x16, 0x00, 0x04, 0x01, 0x04 };
  uint8_t startAccel[] = { 0x02, 0x02, 0x00, 0x01, 0x34, 0x00, 0x01, 0x01, 0x10, 0x00, 0x02, 0x01, 0x08, 0x00, 0x04, 0x01, 0x03 };
  uint8_t startPpi[] = { 0x02, 0x03 };
  uint8_t startSdk[] = { 0x02, 0x09 };

  // Serial.println("Fetching Accel State");
  // pControlCharacteristic->writeValue(getAccel, sizeof(getAccel), true);
  // delay(1000);

  // Serial.println("Fetching PPG State with response");
  // pControlCharacteristic->writeValue(getPpg, sizeof(getPpg), true);
  // delay(1000);

  // Serial.println("Entering SDK Mode");
  // pControlCharacteristic->writeValue(startSdk, sizeof(startSdk), true);
  // delay(1000);  // Wait for SDK mode to be enabled

  // Serial.println("Fetching PPG State");
  // pControlCharacteristic->writeValue(getPpg, sizeof(getPpg), true);
  // delay(1000);

  // Serial.println("Starting PPG Measurements");
  // pControlCharacteristic->writeValue(startPpg, sizeof(startPpg), true);
  // delay(1000);

  // Serial.println("Fetching PPG State");
  // pControlCharacteristic->writeValue(getPpg, sizeof(getPpg), true);
  // delay(1000);

  // Serial.println("Fetching PPG State");
  // pControlCharacteristic->writeValue(getPpg, sizeof(getPpg), true);
  // delay(1000);

  Serial.println("Starting PPI Measurements");
  pControlCharacteristic->writeValue(startPpi, sizeof(startPpi), true);
  delay(1000);

  Serial.println("Fetching PPI State");
  pControlCharacteristic->writeValue(getPpi, sizeof(getPpi), true);
  delay(1000);

  // Send [0x01 0x01] (read ppg settings) in normal mode
  // f0 01 01 - control point response for read ppg (0x01 0x01)
  // 00 00    - no errors or more frames 
  // 00 01    - setting 0x00 (sample rate 2 bytes, array length 1)
  // 37 00    - sample rate is 0x37 = 55Hz
  // 01 01    - setting 0x01 (RES_BYTES 2 bytes, array length 1)
  // 16 00    - RES_BYTES is 0x16 = 22
  // 04 01    - setting 0x04 (number of channels 1 byte, array length 1)
  // 04       - 4 channels (red, green, infrared, ambient)

  // Send [0x01 0x01] (read ppg settings) in sdk mode
  // f0 01 01 - control point response for read (0x01) ppg (0x01)
  // 00 00    - no errors or more frames
  // 00 05    - setting 0x00 (available sample rates 2 bytes each, array length 5)
  // 1c 00    - sample rate 1 (0x1c = 28 Hz)
  // 2c 00    - sample rate 2 (0x2c = 44 Hz)
  // 37 00    - sample rate 3 (0x37 = 55 Hz)
  // 87 00    - sample rate 4 (0x87 = 135 Hz)
  // b0 00    - sample rate 5 (0xb0 = 176 Hz)
  // 01 01    - setting 0x01 (RES_BYTES 2 bytes, array length 1)
  // 16 00    - resolution is 0x16 = 22
  // 04 01    - setting 0x04 (number of channels 1 byte, array length 1)
  // 04       - 4 channels (red, green, infrared, ambient)

  // Available sampling rates in SDK: 28Hz, 44Hz, 55Hz, 135Hz, and 176Hz
  // In hex these are: 0x1c, 0x2c, 0x37, 0x87, 0xb0 (add suffix 0x00 byte for 2 byte width)

  // Start PPG stream command
  // 02 01    - start measurement (0x02) of ppg (0x01)
  // 00 01    - setting 0x00 length 0x01 (sample rate)
  // 87 00    - choose 135Hz (0x87)
  // 01 01    - setting 0x01 length 0x01 (RES_BYTES)
  // 16 00    - choose RES_BYTES 0x16 (default?)
  // don't pass range settings (0x02, 0x03)?
  // 04 01    - setting 0x04 (number of channels)
  // 04
  // Full command:
  // {0x02, 0x01, 0x00, 0x01, 0x87, 0x00, 0x01, 0x01, 0x16, 0x00, 0x04, 0x01, 0x04}

  // Settings after starting the PPG stream:
  // Response:
  // f0 01 01 00 00  00 05 1c 00 2c 00 37 00 87 00 b0 00 01 01 16 00 04 01 04
  // Same as response before starting stream in sdk mode

  connected = true;
  return true;
}
