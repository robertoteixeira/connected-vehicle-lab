#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Wire.h>

#include "BleProvisioningService.h"
#include "BleVehicleService.h"
#include "DebouncedButton.h"
#include "VehicleState.h"

constexpr uint8_t IGNITION_BUTTON_PIN = 27;
constexpr uint8_t THROTTLE_PIN = 34;
constexpr uint8_t BRAKE_BUTTON_PIN = 26;
constexpr uint8_t HEADLIGHT_BUTTON_PIN = 25;
constexpr unsigned long BUTTON_DEBOUNCE_MS = 50;
constexpr float IDLE_RPM = 800.0F;
constexpr float MAX_RPM = 6000.0F;
constexpr float MAX_SPEED_KPH = 180.0F;
constexpr float ACCELERATION_KPH_PER_SECOND = 30.0F;
constexpr float DECELERATION_KPH_PER_SECOND = 20.0F;
constexpr unsigned long TELEMETRY_INTERVAL_MS = 500;
constexpr unsigned long DISPLAY_INTERVAL_MS = 100;
constexpr float THROTTLE_DEADBAND_PERCENT = 0.5F;
constexpr float THROTTLE_MIN_PERCENT = 1.0F;
constexpr float THROTTLE_MAX_PERCENT = 99.0F;
constexpr float BRAKING_KPH_PER_SECOND = 60.0F;

constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;
constexpr int OLED_RESET_PIN = -1;
constexpr uint8_t OLED_ADDRESS = 0x3C;

DeviceIdentity deviceIdentity;
BleProvisioningService bleProvisioningService;
VehicleState vehicleState;
DebouncedButton ignitionButton(IGNITION_BUTTON_PIN, BUTTON_DEBOUNCE_MS);
DebouncedButton brakeButton(BRAKE_BUTTON_PIN, BUTTON_DEBOUNCE_MS);
DebouncedButton headlightButton(HEADLIGHT_BUTTON_PIN, BUTTON_DEBOUNCE_MS);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);
BleVehicleService bleVehicleService;

unsigned long lastSimulationUpdateMs = 0;
unsigned long lastTelemetryMs = 0;
unsigned long lastDisplayUpdateMs = 0;
bool lastIgnitionPressed = false;
bool lastHeadlightPressed = false;

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
    Serial.printf("Throttle: %.1f%%, RPM: %.0f, Speed: %.1f km/h, Brake: %s, Headlights: %s\n",
                  vehicleState.throttlePosition, vehicleState.engineRpm, vehicleState.speedKph,
                  vehicleState.brakePressed ? "ON" : "OFF",
                  vehicleState.headlightsOn ? "ON" : "OFF");
}

void scanI2cBus() {
    Serial.println("Scanning I2C bus...");

    for (uint8_t address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        const uint8_t error = Wire.endTransmission();

        if (error == 0) {
            Serial.printf("I2C device found at address 0x%02X\n", address);
        }
    }
}

void updateDisplay() {
    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);

    display.println("CONNECTED VEHICLE");

    display.println();
    display.setTextSize(2);
    display.printf("%.0f km/h", vehicleState.speedKph);

    display.println();
    display.setTextSize(1);
    display.printf("RPM: %.0f", vehicleState.engineRpm);

    display.println();
    display.printf("IGN:%s BRK:%s LGT:%s", vehicleState.ignitionOn ? "ON" : "OFF",
                   vehicleState.brakePressed ? "ON" : "OFF",
                   vehicleState.headlightsOn ? "ON" : "OFF");

    display.display();
}

void setup() {
    ignitionButton.begin();
    brakeButton.begin();
    headlightButton.begin();

    Serial.begin(115200);

    BLEDevice::init("ConnectedVehicle");
    BLEServer* bleServer = BLEDevice::createServer();

    bleVehicleService.begin(bleServer, vehicleState);
    bleProvisioningService.begin(bleServer, deviceIdentity);

    Wire.begin(21, 22);

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println("OLED initialization failed");
    }

    updateDisplay();

    scanI2cBus();

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
    headlightButton.update();
    const bool headlightPressed = headlightButton.isPressed();

    const unsigned long now = millis();

    if (buttonPressed && !lastIgnitionPressed) {
        vehicleState.ignitionOn = !vehicleState.ignitionOn;
        Serial.printf("Ignition: %s\n", vehicleState.ignitionOn ? "ON" : "OFF");
    }

    lastIgnitionPressed = buttonPressed;

    if (headlightPressed && !lastHeadlightPressed) {
        vehicleState.headlightsOn = !vehicleState.headlightsOn;
        Serial.printf("Headlights: %s\n", vehicleState.headlightsOn ? "ON" : "OFF");
    }

    lastHeadlightPressed = headlightPressed;

    updateThrottleInput();

    const float deltaTimeSeconds = static_cast<float>(now - lastSimulationUpdateMs) / 1000.0F;
    lastSimulationUpdateMs = now;
    updateVehicleSimulation(deltaTimeSeconds);

    if ((now - lastDisplayUpdateMs) >= DISPLAY_INTERVAL_MS) {
        lastDisplayUpdateMs = now;
        updateDisplay();
    }

    if ((now - lastTelemetryMs) >= TELEMETRY_INTERVAL_MS) {
        lastTelemetryMs = now;

        bleVehicleService.update(vehicleState);
        printTelemetry();
    }
}