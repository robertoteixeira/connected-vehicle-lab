#pragma once

struct VehicleState {
    bool ignitionOn = false;
    bool brakePressed = false;

    float speedKph = 0.0F;
    float engineRpm = 0.0F;
    float throttlePosition = 0.0F;

    float coolantTemperatureC = 20.0F;
    float batteryVoltage = 12.6F;
};