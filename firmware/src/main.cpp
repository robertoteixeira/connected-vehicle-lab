#include <Arduino.h>

#include "DebouncedButton.h"
#include "VehicleState.h"

constexpr uint8_t IGNITION_BUTTON_PIN = 27;
constexpr uint8_t THROTTLE_PIN = 34;
constexpr uint8_t BRAKE_BUTTON_PIN = 26;
constexpr unsigned long BUTTON_DEBOUNCE_MS = 50;
constexpr float IDLE_RPM = 800.0F;
constexpr float MAX_RPM = 6000.0F;
constexpr float MAX_SPEED_KPH = 180.0F;
constexpr float ACCELERATION_KPH_PER_SECOND = 30.0F;
constexpr float DECELERATION_KPH_PER_SECOND = 20.0F;
constexpr unsigned long TELEMETRY_INTERVAL_MS = 500;
constexpr float THROTTLE_DEADBAND_PERCENT = 0.5F;
constexpr float THROTTLE_MIN_PERCENT = 1.0F;
constexpr float THROTTLE_MAX_PERCENT = 99.0F;
constexpr float BRAKING_KPH_PER_SECOND = 60.0F;

VehicleState vehicleState;
DebounceButton ignitionButton(IGNITION_BUTTON_PIN, BUTTON_DEBOUNCE_MS);
DebounceButton brakeButton(BRAKE_BUTTON_PIN, BUTTON_DEBOUNCE_MS);

unsigned long lastSimulationUpdateMs = 0;
unsigned long lastTelemetryMs = 0;
bool lastIgnitionPressed = false;

void updateThrottleInput() {
    constexpr int SAMPLE_COUNT = 16;

    int throttleRawSum = 0;

    for (int i = 0; i < SAMPLE_COUNT; ++i) {
        throttleRawSum += analogRead(THROTTLE_PIN);
    }

    const int throttleRaw = throttleRawSum / SAMPLE_COUNT;

    float throttlePercent = (static_cast<float>(throttleRaw) / 4095.0F) * 100.0F;

    if (throttlePercent <= THROTTLE_MIN_PERCENT) {
        throttlePercent = 0.0F;
    } else if (throttlePercent >= THROTTLE_MAX_PERCENT) {
        throttlePercent = 100.0F;
    }

    const float throttleDifference = fabsf(throttlePercent - vehicleState.throttlePosition);

    if (throttleDifference > THROTTLE_DEADBAND_PERCENT) {
        vehicleState.throttlePosition = throttlePercent;
    }
}

void updateVehicleSimulation(float deltaTimeSeconds) {
    if (vehicleState.ignitionOn) {
        vehicleState.engineRpm =
            IDLE_RPM + (vehicleState.throttlePosition / 100.0F) * (MAX_RPM - IDLE_RPM);

        const float targetSpeedKph = (vehicleState.throttlePosition / 100.0F) * MAX_SPEED_KPH;
        const float accelerationStep = ACCELERATION_KPH_PER_SECOND * deltaTimeSeconds;
        const float decelerationStep = DECELERATION_KPH_PER_SECOND * deltaTimeSeconds;
        const float brakingStep = BRAKING_KPH_PER_SECOND * deltaTimeSeconds;

        if (vehicleState.brakePressed) {
            vehicleState.speedKph -= brakingStep;

            if (vehicleState.speedKph < 0.0F) {
                vehicleState.speedKph = 0.0F;
            }

            return;
        }

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
    Serial.printf("Throttle: %.1f%%, RPM: %.0f, Speed: %.1f km/h, Brake: %s\n",
                  vehicleState.throttlePosition, vehicleState.engineRpm, vehicleState.speedKph,
                  vehicleState.brakePressed ? "ON" : "OFF");
}

void setup() {
    ignitionButton.begin();
    brakeButton.begin();

    Serial.begin(115200);

    Serial.println("Connected Vehicle Lab");
    Serial.println("ESP32 booted successfully");

    Serial.printf("Vehicle state: ignition=%s, speed=%.1f km/h, rpm=%.0f, throttle=%.1f%%\n",
                  vehicleState.ignitionOn ? "ON" : "OFF", vehicleState.speedKph,
                  vehicleState.engineRpm, vehicleState.throttlePosition);
}

void loop() {
    ignitionButton.update();
    const bool buttonPressed = ignitionButton.isPressed();
    brakeButton.update();
    vehicleState.brakePressed = brakeButton.isPressed();

    const unsigned long now = millis();

    if (buttonPressed && !lastIgnitionPressed) {
        vehicleState.ignitionOn = !vehicleState.ignitionOn;
        Serial.printf("Ignition: %s\n", vehicleState.ignitionOn ? "ON" : "OFF");
    }

    lastIgnitionPressed = buttonPressed;

    updateThrottleInput();

    const float deltaTimeSeconds = static_cast<float>(now - lastSimulationUpdateMs) / 1000.0F;
    lastSimulationUpdateMs = now;
    updateVehicleSimulation(deltaTimeSeconds);

    if ((now - lastTelemetryMs) >= TELEMETRY_INTERVAL_MS) {
        lastTelemetryMs = now;
        printTelemetry();
    }
}