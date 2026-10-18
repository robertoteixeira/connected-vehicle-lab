#pragma once

#include <BLEDevice.h>

#include "VehicleState.h"

class BleVehicleService {
   public:
    void begin(const VehicleState& vehicleState);
    void udpdate(const VehicleState& vehicleState);

   private:
    BLECharacteristic* vehicleStateCharacteristic_ = nullptr;
};