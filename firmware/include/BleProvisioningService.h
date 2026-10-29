#pragma once

#include <BLEDevice.h>

#include "DeviceIdentity.h"
#include "WifiManager.h"

class BleProvisioningService {
   public:
    void begin(BLEServer* server, const DeviceIdentity& identity, WifiManager& wifiManager);
    void update(WifiProvisioningState state);

   private:
    BLECharacteristic* deviceIdentityCharacteristic_ = nullptr;
    BLECharacteristic* wifiConfigCharacteristic_ = nullptr;
    BLECharacteristic* wifiStatusCharacteristic_ = nullptr;
    WifiProvisioningState lastWifiState_ = WifiProvisioningState::Idle;
};