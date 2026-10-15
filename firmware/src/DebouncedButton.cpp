#include "DebouncedButton.h"

DebouncedButton::DebouncedButton(uint8_t pin, unsigned long debounceMs)
    : pin_(pin), debounceMs_(debounceMs) {}

void DebouncedButton::begin() { pinMode(pin_, INPUT_PULLUP); }

void DebouncedButton::update() {
    const bool rawPressed = digitalRead(pin_) == LOW;
    const unsigned long now = millis();

    if (rawPressed != lastRawPressed_) {
        lastChangeMs_ = now;
        lastRawPressed_ = rawPressed;
    }

    if ((now - lastChangeMs_) >= debounceMs_) {
        stablePressed_ = rawPressed;
    }
}

bool DebouncedButton::isPressed() const { return stablePressed_; }