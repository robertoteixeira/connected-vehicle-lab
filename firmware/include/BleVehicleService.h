#pragma once

#include <BLEDevice.h>

#include "VehicleState.h"

class BleVehicleService {
   public:
    void begin(const VehicleState& vehicleState);
    void update(const VehicleState& vehicleState);

   private:
    void serializeVehicleState(const VehicleState& vehicleState, char* payload, size_t payloadSize);
    BLECharacteristic* vehicleStateCharacteristic_ = nullptr;
    char lastPayload_[128] = {};
};