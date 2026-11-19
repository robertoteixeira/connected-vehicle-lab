#pragma once

#include <mqtt_client.h>

class AzureIoTManager {
   public:
    bool begin();
    void update();

   private:
    esp_mqtt_client_handle_t client_ = nullptr;
    bool timeSyncStarted_ = false;
    bool clockReady_ = false;
    bool mqttStarted_ = false;
    unsigned long lastConnectionAttemptMs_ = 0;
};