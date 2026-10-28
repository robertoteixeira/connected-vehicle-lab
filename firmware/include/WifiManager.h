#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

enum class WifiProvisioningState { Idle, Connecting, Connected, TimedOut };

class WifiManager {
   public:
    void begin();
    void connect(const char* ssid, const char* password);
    void update();
    WifiProvisioningState provisioningState() const;

   private:
    struct ConnectionRequest {
        char ssid[33];
        char password[65];
    };

    QueueHandle_t connectionQueue_ = nullptr;
    WifiProvisioningState provisioningState_ = WifiProvisioningState::Idle;
    unsigned long connectionStartedMs_ = 0;
};