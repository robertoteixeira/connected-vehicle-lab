#include "BleVehicleService.h"

#include <BLE2902.h>
#include <BLEDevice.h>

#include <cstdio>
#include <cstring>

constexpr char DEVICE_NAME[] = "ConnectedVehicle";
constexpr char VEHICLE_SERVICE_UUID[] = "7b9f0001-6c5a-4e8f-9d3a-2f1b7c8e0001";
constexpr char VEHICLE_STATE_CHARACTERISTIC_UUID[] = "7b9f0002-6c5a-4e8f-9d3a-2f1b7c8e0001";

void BleVehicleService::begin(BLEServer* server, const VehicleState& vehicleState) {
    BLEService* vehicleService = server->createService(VEHICLE_SERVICE_UUID);

    vehicleStateCharacteristic_ = vehicleService->createCharacteristic(
        VEHICLE_STATE_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

    vehicleStateCharacteristic_->addDescriptor(new BLE2902());

    char payload[128];
    serializeVehicleState(vehicleState, payload, sizeof(payload));

    vehicleStateCharacteristic_->setValue(payload);
    strncpy(lastPayload_, payload, sizeof(lastPayload_) - 1);
    lastPayload_[sizeof(lastPayload_) - 1] = '\0';

    vehicleService->start();

    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(VEHICLE_SERVICE_UUID);
    advertising->start();
}

void BleVehicleService::update(const VehicleState& vehicleState) {
    char payload[128];
    serializeVehicleState(vehicleState, payload, sizeof(payload));

    if (strcmp(payload, lastPayload_) == 0) {
        return;
    }

    vehicleStateCharacteristic_->setValue(payload);
    vehicleStateCharacteristic_->notify();

    strncpy(lastPayload_, payload, sizeof(lastPayload_) - 1);
    lastPayload_[sizeof(lastPayload_) - 1] = '\0';
}

void BleVehicleService::serializeVehicleState(const VehicleState& vehicleState, char* payload,
                                              size_t payloadSize) {
    snprintf(payload, payloadSize, "ign=%d,speed=%.1f,rpm=%.0f,thr=%.1f,brk=%d,lgt=%d",
             vehicleState.ignitionOn, vehicleState.speedKph, vehicleState.engineRpm,
             vehicleState.throttlePosition, vehicleState.brakePressed, vehicleState.headlightsOn);
}