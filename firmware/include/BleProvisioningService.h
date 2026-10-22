#pragma once

#include <BLEDevice.h>

#include "DeviceIdentity.h"

class BleProvisioningService {
   public:
    void begin(BLEServer* server, const DeviceIdentity& identity);

   private:
    BLECharacteristic* deviceIdentityCharacteristic_ = nullptr;
};