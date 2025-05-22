#ifndef _POLARBLECONNECT_H_
#define _POLARBLECONNECT_H_

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct ppi_data {
  unsigned long timestamp;
  uint8_t  heartRate;
  uint16_t ppi;
  uint16_t ppError;
  uint8_t  flags;
  bool     valid;
} PPIData;

class PolarBLEConnection {
  public:
    static boolean connected;
    static boolean doConnect;
    static boolean doScan;

    static BLERemoteCharacteristic* pControlCharacteristic;
    static BLERemoteCharacteristic* pDataCharacteristic;
    static BLEAdvertisedDevice* myDevice;

    static BLEUUID serviceUUID;
    static BLEUUID controlCharUUID;
    static BLEUUID dataCharUUID;

    static QueueHandle_t ppiQueue;

    // Default constructor
    PolarBLEConnection();

    // Parameterized constructor
    PolarBLEConnection(String serviceUUID, String controlCharUUID, String dataCharUUID);

    // Callback functions
    // Parses the data from the control characteristic for all notifications
    void NotifyCallback(
      BLERemoteCharacteristic* pBLERemoteCharacteristic,
      uint8_t* pData,
      size_t length,
      bool isNotify);

    // Parses the PPG data
    void PpgNotifyCallback(
      uint8_t* &pData,
      size_t &length);

    // Parses the PPI data
    void PpiNotifyCallback(
      uint8_t* &pData,
      size_t &length);

    // Read data from the control characteristic and then
    // print it to the Serial Monitor in raw hexadecimal format
    void ReadData();

    // Connect to the Polar device. If successful, set the connected
    // flag to true and return true. Otherwise, return false.
    bool ConnectToServer();

  // Class definitions
  class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
      connected = true;
    }

    void onDisconnect(BLEClient* pclient) {
      connected = false;
      Serial.println("Disconnected");
    }
  };  // class MyClientCallback

  class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    public:
      MyAdvertisedDeviceCallbacks(String deviceName) : device_name_(deviceName) {}

      ~MyAdvertisedDeviceCallbacks() {}

      String deviceName() {
        return device_name_;
      }

      void onResult(BLEAdvertisedDevice advertisedDevice) {
        Serial.println("Found device: " + advertisedDevice.toString());
        delay(20);
        if (advertisedDevice.getName().indexOf(device_name_) != -1) {
          Serial.println("Found device: " + advertisedDevice.toString());
          BLEDevice::getScan()->stop();
          if (myDevice != nullptr) {
            delete myDevice;
            myDevice = nullptr;
          }
          myDevice = new BLEAdvertisedDevice(advertisedDevice);
          doConnect = true;
        }
        // else we do not care about the advertised device, so do nothing.
      }

    private:
      void processPpgData(uint64_t time,
                                 float ppgGrn,
                                 float ppgRed,
                                 float ppgInf,
                                 float ppgAmb);

      String device_name_;
  };  // class MyAdvertisedDeviceCallbacks

};  // class PolarBLEConnection

#endif  // _POLARBLECONNECT_H_
