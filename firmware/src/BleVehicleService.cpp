#include "BleVehicleService.h"

#include <cstdio>
#include <cstring>

constexpr char VEHICLE_SERVICE_UUID[] = "7b9f0001-6c5a-4e8f-9d3a-2f1b7c8e0001";
constexpr char VEHICLE_STATE_CHARACTERISTIC_UUID[] = "7b9f0002-6c5a-4e8f-9d3a-2f1b7c8e0001";

void BleVehicleService::begin(NimBLEServer* server, const VehicleState& vehicleState) {
    NimBLEService* vehicleService = server->createService(VEHICLE_SERVICE_UUID);

    vehicleStateCharacteristic_ = vehicleService->createCharacteristic(
        VEHICLE_STATE_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

    char payload[128];
    serializeVehicleState(vehicleState, payload, sizeof(payload));

    vehicleStateCharacteristic_->setValue(payload);
    strncpy(lastPayload_, payload, sizeof(lastPayload_) - 1);
    lastPayload_[sizeof(lastPayload_) - 1] = '\0';
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