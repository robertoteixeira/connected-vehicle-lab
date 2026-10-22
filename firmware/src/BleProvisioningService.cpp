#include "BleProvisioningService.h"

constexpr char PROVISIONING_SERVICE_UUID[] = "7b9f0010-6c5a-4e8f-9d3a-2f1b7c8e0001";
constexpr char DEVICE_IDENTITY_CHARACTERISTIC_UUID[] = "7b9f0011-6c5a-4e8f-9d3a-2f1b7c8e0001";

void BleProvisioningService::begin(BLEServer* server, const DeviceIdentity& identity) {
    BLEService* provisioningService = server->createService(PROVISIONING_SERVICE_UUID);

    deviceIdentityCharacteristic_ = provisioningService->createCharacteristic(
        DEVICE_IDENTITY_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ);

    char payload[128];

    snprintf(payload, sizeof(payload), "id=%s,hw=%s,fw=%s", identity.deviceId,
             identity.hardwareVersion, identity.firmwareVersion);

    deviceIdentityCharacteristic_->setValue(payload);

    provisioningService->start();
}
