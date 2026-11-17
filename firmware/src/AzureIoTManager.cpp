#include "AzureIoTManager.h"

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include "AzureIoTConfig.h"

bool AzureIoTManager::begin() {
    if (client_ != nullptr) {
        return true;
    }

    esp_mqtt_client_config_t config = {};
    config.host = AzureIoTConfig::HOST_NAME;
    config.port = 8883;
    config.transport = MQTT_TRANSPORT_OVER_SSL;
    config.client_id = AzureIoTConfig::DEVICE_ID;

    client_ = esp_mqtt_client_init(&config);

    return client_ != nullptr;
}

void AzureIoTManager::update() {
    if (client_ == nullptr || WiFi.status() != WL_CONNECTED) {
        return;
    }

    if (!timeSyncStarted_) {
        configTime(0, 0, "pool.ntp.org", "time.google.com");
        timeSyncStarted_ = true;

        Serial.println("Clock synchronization requested");
    }

    if (!clockReady_) {
        constexpr time_t MIN_VALID_TIME = 1704067200;

        if (time(nullptr) >= MIN_VALID_TIME) {
            clockReady_ = true;
            Serial.println("Clock ready for Azure connection");
        }
    }
}