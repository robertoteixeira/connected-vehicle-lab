#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

class WifiManager {
   public:
    void begin();
    void connect(const char* ssid, const char* password);
    void update();

   private:
    struct ConnectionRequest {
        char ssid[33];
        char password[65];
    };

    QueueHandle_t connectionQueue_ = nullptr;
    bool connecting_ = false;
    unsigned long connectionStartedMs_ = 0;
};