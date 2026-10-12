#include <Arduino.h>

#include "VehicleState.h"

constexpr uint8_t IGNITION_BUTTON_PIN = 27;
constexpr uint8_t THROTTLE_PIN = 34;
constexpr unsigned long BUTTON_DEBOUNCE_MS = 50;
constexpr float IDLE_RPM = 800.0F;
constexpr float MAX_RPM = 6000.0F;
constexpr float MAX_SPEED_KPH = 180.0F;
constexpr float ACCELERATION_KPH_PER_SECOND = 30.0F;
constexpr float DECELERATION_KPH_PER_SECOND = 20.0F;
constexpr unsigned long TELEMETRY_INTERVAL_MS = 500;

VehicleState vehicleState;
bool lastRawButtonPressed = false;
bool stableButtonPressed = false;
unsigned long lastButtonChangeMs = 0;
unsigned long lastSimulationUpdateMs = 0;
unsigned long lastTelemetryMs = 0;

void updateThrottleInput() {
    const int throttleRaw = analogRead(THROTTLE_PIN);

    vehicleState.throttlePosition = (static_cast<float>(throttleRaw) / 4095.0F) * 100.0F;
}

void updateVehicleSimulation(float deltaTimeSeconds) {
    if (vehicleState.ignitionOn) {
        vehicleState.engineRpm =
            IDLE_RPM + (vehicleState.throttlePosition / 100.0F) * (MAX_RPM - IDLE_RPM);

        const float targetSpeedKph = (vehicleState.throttlePosition / 100.0F) * MAX_SPEED_KPH;
        const float accelerationStep = ACCELERATION_KPH_PER_SECOND * deltaTimeSeconds;
        const float decelerationStep = DECELERATION_KPH_PER_SECOND * deltaTimeSeconds;

        if (vehicleState.speedKph < targetSpeedKph) {
            vehicleState.speedKph += accelerationStep;

            if (vehicleState.speedKph > targetSpeedKph) {
                vehicleState.speedKph = targetSpeedKph;
            }
        } else if (vehicleState.speedKph > targetSpeedKph) {
            vehicleState.speedKph -= decelerationStep;

            if (vehicleState.speedKph < targetSpeedKph) {
                vehicleState.speedKph = targetSpeedKph;
            }
        }
    } else {
        vehicleState.engineRpm = 0.0F;
        vehicleState.speedKph = 0.0F;
    }
}

void printTelemetry() {
    Serial.printf("Throttle: %.1f%%, RPM: %.0f, Speed: %.1f km/h\n", vehicleState.throttlePosition,
                  vehicleState.engineRpm, vehicleState.speedKph);
}

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
                Serial.printf("Ignition: %s\n", vehicleState.ignitionOn ? "ON" : "OFF");
            }
        }
    }

    updateThrottleInput();

    const float deltaTimeSeconds = static_cast<float>(now - lastSimulationUpdateMs) / 1000.0F;
    lastSimulationUpdateMs = now;
    updateVehicleSimulation(deltaTimeSeconds);

    if ((now - lastTelemetryMs) >= TELEMETRY_INTERVAL_MS) {
        lastTelemetryMs = now;
        printTelemetry();
    }
}