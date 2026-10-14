#pragma once

#include <Arduino.h>

class DebounceButton {
   public:
    DebounceButton(uint8_t pin, unsigned long debounceMs);

    void begin();
    void update();
    bool isPressed() const;

   private:
    uint8_t pin_;
    unsigned long debounceMs_;

    bool lastRawPressed_ = false;
    bool stablePressed_ = false;
    unsigned long lastChangeMs_ = 0;
};