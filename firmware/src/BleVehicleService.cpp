#include "BleVehicleService.h"

#include <BLEDevice.h>

constexpr char DEVICE_NAME[] = "ConnectedVehicle";
constexpr char VEHICLE_SERVICE_UUID[] = "7b9f0001-6c5a-4e8f-9d3a-2f1b7c8e0001";
constexpr char VEHICLE_STATE_CHARACTERISTIC_UUID[] = "7b9f0002-6c5a-4e8f-9d3a-2f1b7c8e0001";

void BleVehicleService::begin(const VehicleState& vehicleState) {
    BLEDevice::init(DEVICE_NAME);

    BLEServer* server = BLEDevice::createServer();
    BLEService* vehicleService = server->createService(VEHICLE_SERVICE_UUID);

    vehicleStateCharacteristic_ = vehicleService->createCharacteristic(
        VEHICLE_STATE_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    vehicleStateCharacteristic_->setValue(vehicleState.ignitionOn ? "ON" : "OFF");

    vehicleService->start();

    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(VEHICLE_SERVICE_UUID);
    advertising->start();
}

void BleVehicleService::udpdate(const VehicleState& vehicleState) {
    vehicleStateCharacteristic_->setValue(vehicleState.ignitionOn ? "ON" : "OFF");
    vehicleStateCharacteristic_->notify();
}