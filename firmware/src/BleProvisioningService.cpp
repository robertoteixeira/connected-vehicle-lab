#include "BleProvisioningService.h"

#include <Arduino.h>

constexpr char PROVISIONING_SERVICE_UUID[] = "7b9f0010-6c5a-4e8f-9d3a-2f1b7c8e0001";
constexpr char DEVICE_IDENTITY_CHARACTERISTIC_UUID[] = "7b9f0011-6c5a-4e8f-9d3a-2f1b7c8e0001";
constexpr char WIFI_CONFIG_CHARACTERISTIC_UUID[] = "7b9f0012-6c5a-4e8f-9d3a-2f1b7c8e0001";
constexpr char WIFI_STATUS_CHARACTERISTIC_UUID[] = "7b9f0013-6c5a-4e8f-9d3a-2f1b7c8e0001";
constexpr char WIFI_REQUEST_RESULT_CHARACTERISTIC_UUID[] = "7b9f0014-6c5a-4e8f-9d3a-2f1b7c8e0001";

class WifiConfigCallbacks : public NimBLECharacteristicCallbacks {
   public:
    WifiConfigCallbacks(WifiManager& wifiManager, NimBLECharacteristic& requestResult)
        : wifiManager_(wifiManager), requestResult_(requestResult) {}

    void onWrite(NimBLECharacteristic* characteristic, NimBLEConnInfo& connInfo) override {
        const std::string value = characteristic->getValue();

        if (value.empty() || value.find('\0') != std::string::npos) {
            Serial.println("Invalid Wi-Fi configuration payload");
            reportResult("rejected");
            return;
        }

        const std::string ssidPrefix = "ssid=";
        const std::string passwordSeparator = ",password=";

        const size_t passwordPosition = value.find(passwordSeparator);

        if (passwordPosition == std::string::npos || value.rfind(ssidPrefix, 0) != 0) {
            Serial.println("Invalid Wi-Fi configuration payload");
            reportResult("rejected");
            return;
        }

        const std::string ssid =
            value.substr(ssidPrefix.length(), passwordPosition - ssidPrefix.length());

        const std::string password = value.substr(passwordPosition + passwordSeparator.length());

        if (ssid.empty() || ssid.size() > 32 || password.size() > 64) {
            Serial.println("Invalid Wi-Fi credential length");
            reportResult("rejected");
            return;
        }

        const bool accepted = wifiManager_.connect(ssid.c_str(), password.c_str());

        reportResult(accepted ? "accepted" : "rejected");
    }

   private:
    WifiManager& wifiManager_;
    NimBLECharacteristic& requestResult_;

    void reportResult(const char* result) {
        requestResult_.setValue(result);
        requestResult_.notify();

        Serial.printf("Wi-Fi request result: %s\n", result);
    }
};

void BleProvisioningService::begin(NimBLEServer* server, const DeviceIdentity& identity,
                                   WifiManager& wifiManager) {
    NimBLEService* provisioningService = server->createService(PROVISIONING_SERVICE_UUID);

    deviceIdentityCharacteristic_ = provisioningService->createCharacteristic(
        DEVICE_IDENTITY_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::READ);

    wifiConfigCharacteristic_ = provisioningService->createCharacteristic(
        WIFI_CONFIG_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE);

    wifiRequestResultCharacteristic_ = provisioningService->createCharacteristic(
        WIFI_REQUEST_RESULT_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

    wifiRequestResultCharacteristic_->setValue("idle");

    wifiConfigCharacteristic_->setCallbacks(
        new WifiConfigCallbacks(wifiManager, *wifiRequestResultCharacteristic_));

    char payload[128];

    snprintf(payload, sizeof(payload), "id=%s,hw=%s,fw=%s", identity.deviceId,
             identity.hardwareVersion, identity.firmwareVersion);

    deviceIdentityCharacteristic_->setValue(payload);

    wifiStatusCharacteristic_ = provisioningService->createCharacteristic(
        WIFI_STATUS_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

    wifiStatusCharacteristic_->setValue("idle");
}

void BleProvisioningService::update(WifiProvisioningState state) {
    if (wifiStatusCharacteristic_ == nullptr || state == lastWifiState_) {
        return;
    }

    const char* payload = nullptr;

    switch (state) {
        case WifiProvisioningState::Idle:
            payload = "idle";
            break;
        case WifiProvisioningState::Connecting:
            payload = "connecting";
            break;
        case WifiProvisioningState::Connected:
            payload = "connected";
            break;
        case WifiProvisioningState::TimedOut:
            payload = "timed_out";
            break;
    }

    if (payload == nullptr) {
        return;
    }

    wifiStatusCharacteristic_->setValue(payload);
    wifiStatusCharacteristic_->notify();

    lastWifiState_ = state;

    Serial.printf("Wi-Fi provisioning status: %s\n", payload);
}
