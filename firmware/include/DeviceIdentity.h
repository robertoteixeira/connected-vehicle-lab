#pragma once

struct DeviceIdentity {
    char deviceId[17] = {};
    const char* hardwareVersion = "ESP32-DEVKIT-V1";
    const char* firmwareVersion = "0.1.0";
};