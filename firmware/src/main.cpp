#include <Arduino.h>

#include "VehicleState.h"

constexpr uint8_t IGNITION_BUTTON_PIN = 27;
constexpr uint8_t THROTTLE_PIN = 34;
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

    const int throttleRaw = analogRead(THROTTLE_PIN);
    vehicleState.throttlePosition = (static_cast<float>(throttleRaw) / 4095.0F) * 100.0F;

    if (vehicleState.ignitionOn) {
        vehicleState.engineRpm = 800.0F + (vehicleState.throttlePosition / 100.0F) * 5200.0F;
    } else {
        vehicleState.engineRpm = 0.0F;
    }

    if (buttonPressed != lastRawButtonPressed) {
        lastButtonChangeMs = now;
        lastRawButtonPressed = buttonPressed;
    }

    if ((now - lastButtonChangeMs) >= BUTTON_DEBOUNCE_MS) {
        if (stableButtonPressed != buttonPressed) {
            stableButtonPressed = buttonPressed;

            if (stableButtonPressed) {
                vehicleState.ignitionOn = !vehicleState.ignitionOn;
                Serial.printf("Ignition: %s, RPM: %.0f\n", vehicleState.ignitionOn ? "ON" : "OFF",
                              vehicleState.engineRpm);
            }
        }
    }
}