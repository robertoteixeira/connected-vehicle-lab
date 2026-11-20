#pragma once

#include <NimBLEDevice.h>

#include "VehicleState.h"

class BleVehicleService {
   public:
    void begin(NimBLEServer* server, const VehicleState& vehicleState);
    void update(const VehicleState& vehicleState);

   private:
    void serializeVehicleState(const VehicleState& vehicleState, char* payload, size_t payloadSize);
    NimBLECharacteristic* vehicleStateCharacteristic_ = nullptr;
    char lastPayload_[128] = {};
};