#include "WifiManager.h"

#include <WiFi.h>

#include <cstring>

constexpr unsigned long WIFI_CONNECTION_TIMEOUT_MS = 15000;

void WifiManager::begin() {
    connectionQueue_ = xQueueCreate(1, sizeof(ConnectionRequest));

    if (connectionQueue_ == nullptr) {
        Serial.println("Failed to create Wi-Fi connection queue");
    }
}

void WifiManager::connect(const char* ssid, const char* password) {
    if (connectionQueue_ == nullptr) {
        Serial.println("Wi-Fi connection queue unavailable");
        return;
    }

    if (ssid == nullptr || password == nullptr) {
        Serial.println("Missing Wi-Fi credentials");
        return;
    }

    ConnectionRequest request{};

    const size_t ssidLength = strlen(ssid);
    const size_t passwordLength = strlen(password);

    if (ssidLength == 0 || ssidLength >= sizeof(request.ssid) ||
        passwordLength >= sizeof(request.password)) {
        Serial.println("Invalid Wi-Fi credential length");
        return;
    }

    memcpy(request.ssid, ssid, ssidLength);
    memcpy(request.password, password, passwordLength);

    if (xQueueOverwrite(connectionQueue_, &request) != pdPASS) {
        Serial.println("Failed to queue Wi-Fi connection request");
        return;
    }

    Serial.printf("Wi-Fi connection requested for SSID: %s\n", request.ssid);
}

WifiProvisioningState WifiManager::provisioningState() const { return provisioningState_; }

void WifiManager::update() {
    ConnectionRequest request{};

    if (connectionQueue_ != nullptr && xQueueReceive(connectionQueue_, &request, 0) == pdPASS) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(request.ssid, request.password);

        provisioningState_ = WifiProvisioningState::Connecting;
        connectionStartedMs_ = millis();

        Serial.printf("Connecting to Wi-Fi: %s\n", request.ssid);

        return;
    }

    if (provisioningState_ != WifiProvisioningState::Connecting) {
        return;
    }

    static unsigned long lastStatusLogMs = 0;
    const unsigned long now = millis();

    if (WiFi.status() == WL_CONNECTED) {
        provisioningState_ = WifiProvisioningState::Connected;

        Serial.printf("Wi-Fi connected. IP: %s\n", WiFi.localIP().toString().c_str());

        return;
    }

    if (now - connectionStartedMs_ >= WIFI_CONNECTION_TIMEOUT_MS) {
        provisioningState_ = WifiProvisioningState::TimedOut;
        WiFi.disconnect();

        Serial.println("Wi-Fi connection timed out");

        return;
    }

    if (now - lastStatusLogMs >= 2000) {
        lastStatusLogMs = now;

        Serial.printf("Wi-Fi status: %d\n", static_cast<int>(WiFi.status()));
    }
}