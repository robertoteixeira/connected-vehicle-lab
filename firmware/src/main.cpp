#include <Arduino.h>

#include "VehicleState.h"

constexpr uint8_t IGNITION_BUTTON_PIN = 27;
constexpr unsigned long BUTTON_DEBOUNCE_MS = 50;

VehicleState vehicleState;
bool lastRawButtonPressed = false;
bool stableButtonPressed = false;
unsigned long lastButtonChangeMs = 0;

void setup() {
    pinMode(IGNITION_BUTTON_PIN, INPUT_PULLUP);

    Serial.begin(115200);

    Serial.println("Connected Vehicle Lab");
    Serial.println("ESP32 booted successfully");

    Serial.printf("Vehicle state: ignition=%s, speed=%.1f km/h, rpm=%.0f, throttle=%.1f%%\n",
                  vehicleState.ignitionOn ? "ON" : "OFF", vehicleState.speedKph,
                  vehicleState.engineRpm, vehicleState.throttlePosition);
}

void loop() {
    const bool buttonPressed = digitalRead(IGNITION_BUTTON_PIN) == LOW;
    const unsigned long now = millis();

    if (buttonPressed != lastRawButtonPressed) {
        lastButtonChangeMs = now;
        lastRawButtonPressed = buttonPressed;
    }

    if ((now - lastButtonChangeMs) >= BUTTON_DEBOUNCE_MS) {
        if (stableButtonPressed != buttonPressed) {
            stableButtonPressed = buttonPressed;

            if (stableButtonPressed) {
                vehicleState.ignitionOn = !vehicleState.ignitionOn;
                vehicleState.engineRpm = vehicleState.ignitionOn ? 800.0F : 0.0F;
                Serial.printf("Ignition: %s, RPM: %.0f\n", vehicleState.ignitionOn ? "ON" : "OFF",
                              vehicleState.engineRpm);
            }
        }
    }
}