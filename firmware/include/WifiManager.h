#pragma once

#include <Arduino.h>

class WifiManager {
   public:
    void connect(const char* ssid, const char* password);
    void update();

   private:
    volatile bool connectionRequested_ = false;
    bool connecting_ = false;

    String pendingSsid_;
    String pendingPassword_;
};