#include "BleProvisioningService.h"

#include <Arduino.h>

constexpr char PROVISIONING_SERVICE_UUID[] = "7b9f0010-6c5a-4e8f-9d3a-2f1b7c8e0001";
constexpr char DEVICE_IDENTITY_CHARACTERISTIC_UUID[] = "7b9f0011-6c5a-4e8f-9d3a-2f1b7c8e0001";
constexpr char WIFI_CONFIG_CHARACTERISTIC_UUID[] = "7b9f0012-6c5a-4e8f-9d3a-2f1b7c8e0001";

class WifiConfigCallbacks : public BLECharacteristicCallbacks {
   public:
    explicit WifiConfigCallbacks(WifiManager& wifiManager) : wifiManager_(wifiManager) {}

    void onWrite(BLECharacteristic* characteristic) override {
        const std::string value = characteristic->getValue();

        if (value.empty()) {
            return;
        }

        const std::string ssidPrefix = "ssid=";
        const std::string passwordSeparator = ",password=";

        const size_t passwordPosition = value.find(passwordSeparator);

        if (passwordPosition == std::string::npos || value.rfind(ssidPrefix, 0) != 0) {
            Serial.println("Invalid Wi-Fi configuration payload");
            return;
        }

        const std::string ssid =
            value.substr(ssidPrefix.length(), passwordPosition - ssidPrefix.length());

        const std::string password = value.substr(passwordPosition + passwordSeparator.length());

        Serial.printf("Received Wi-Fi credentials for SSID: %s\n", ssid.c_str());

        wifiManager_.connect(ssid.c_str(), password.c_str());
    }

   private:
    WifiManager wifiManager_;
};

void BleProvisioningService::begin(BLEServer* server, const DeviceIdentity& identity,
                                   WifiManager& wifiManager) {
    BLEService* provisioningService = server->createService(PROVISIONING_SERVICE_UUID);

    deviceIdentityCharacteristic_ = provisioningService->createCharacteristic(
        DEVICE_IDENTITY_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ);
    wifiConfigCharacteristic_ = provisioningService->createCharacteristic(
        WIFI_CONFIG_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
    wifiConfigCharacteristic_->setCallbacks(new WifiConfigCallbacks(wifiManager));

    char payload[128];

    snprintf(payload, sizeof(payload), "id=%s,hw=%s,fw=%s", identity.deviceId,
             identity.hardwareVersion, identity.firmwareVersion);

    deviceIdentityCharacteristic_->setValue(payload);

    provisioningService->start();
}
