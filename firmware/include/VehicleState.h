#pragma once

struct VehicleState {
    bool ignitionOn = false;
    bool brakePressed = false;
    bool headlightsOn = false;

    float speedKph = 0.0F;
    float engineRpm = 0.0F;
    float throttlePosition = 0.0F;
};