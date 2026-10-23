#include "WifiManager.h"

#include <WiFi.h>

void WifiManager::connect(const char* ssid, const char* password) {
    pendingSsid_ = ssid;
    pendingPassword_ = password;
    connectionRequested_ = true;

    Serial.printf("Wi-Fi connection requested for SSID: %s\n", pendingSsid_.c_str());
}

void WifiManager::update() {
    if (connectionRequested_) {
        connectionRequested_ = false;

        WiFi.mode(WIFI_STA);
        WiFi.begin(pendingSsid_.c_str(), pendingPassword_.c_str());

        connecting_ = true;

        Serial.printf("Connecting to Wi-Fi: %s\n", pendingSsid_.c_str());

        return;
    }

    if (!connecting_) {
        return;
    }

    static unsigned long lastStatusLogMs = 0;
    const unsigned long now = millis();

    if (WiFi.status() == WL_CONNECTED) {
        connecting_ = false;

        Serial.printf("Wi-Fi connected. IP: %s\n", WiFi.localIP().toString().c_str());

        return;
    }

    if (now - lastStatusLogMs >= 2000) {
        lastStatusLogMs = now;

        Serial.printf("Wi-Fi status: %d\n", static_cast<int>(WiFi.status()));
    }
}