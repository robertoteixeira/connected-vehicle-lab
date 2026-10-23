#pragma once

#include <BLEDevice.h>

#include "DeviceIdentity.h"
#include "WifiManager.h"

class BleProvisioningService {
   public:
    void begin(BLEServer* server, const DeviceIdentity& identity, WifiManager& wifiManager);

   private:
    BLECharacteristic* deviceIdentityCharacteristic_ = nullptr;
    BLECharacteristic* wifiConfigCharacteristic_ = nullptr;
};