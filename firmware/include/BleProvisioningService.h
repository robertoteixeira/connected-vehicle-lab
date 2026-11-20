#pragma once

#include <NimBLEDevice.h>

#include "DeviceIdentity.h"
#include "WifiManager.h"

class BleProvisioningService {
   public:
    void begin(NimBLEServer* server, const DeviceIdentity& identity, WifiManager& wifiManager);
    void update(WifiProvisioningState state);

   private:
    NimBLECharacteristic* deviceIdentityCharacteristic_ = nullptr;
    NimBLECharacteristic* wifiConfigCharacteristic_ = nullptr;
    NimBLECharacteristic* wifiStatusCharacteristic_ = nullptr;
    WifiProvisioningState lastWifiState_ = WifiProvisioningState::Idle;
    NimBLECharacteristic* wifiRequestResultCharacteristic_ = nullptr;
};