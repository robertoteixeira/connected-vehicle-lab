# Connected Vehicle Lab

An ESP32 vehicle simulation prototype built with C++ and the Arduino framework. Physical controls drive simulated ignition, throttle, braking, and headlights; an OLED shows vehicle state while the serial port emits periodic telemetry.

The current firmware is a standalone hardware lab. It does not yet transmit telemetry over BLE, connect to a mobile app, or interface with a real vehicle or CAN bus.

## Features

- Toggle ignition and headlights with debounced buttons.
- Sample and average an analog throttle input, with a deadband to reduce small fluctuations.
- Simulate engine RPM and speed from throttle position.
- Apply acceleration, coasting deceleration, and braking over elapsed time.
- Display speed, RPM, ignition, brakes, and lights on a 128×64 SSD1306 OLED.
- Print telemetry every 500 ms and refresh the display every 100 ms.
- Scan the I2C bus during startup for wiring diagnostics.

## Hardware and wiring

The PlatformIO environment targets an ESP32 `esp32dev` board.

| Component | ESP32 connection |
| --- | --- |
| Ignition button | GPIO 27 |
| Brake button | GPIO 26 |
| Headlight button | GPIO 25 |
| Throttle potentiometer wiper | GPIO 34 |
| OLED SDA | GPIO 21 |
| OLED SCL | GPIO 22 |
| OLED I2C address | `0x3C` |

Use a common ground. The buttons use the pull-up configuration in `DebouncedButton`, with a press represented by a LOW input; connect each button between its GPIO and GND. Use the ESP32's 3.3 V logic range for the throttle input, with the potentiometer ends connected to 3.3 V and GND. Power the OLED according to its module specifications.

Dependencies are declared in `firmware/platformio.ini`: Adafruit GFX Library and Adafruit SSD1306.

## Build and run

Install PlatformIO Core or its VS Code extension, then:

```sh
git clone https://github.com/robertoteixeira/connected-vehicle-lab.git
cd connected-vehicle-lab/firmware
pio run
pio run --target upload
pio device monitor --baud 115200
```

Specify your board's upload/monitor port if automatic detection fails.

## Try the simulation

1. Open the serial monitor and check the startup messages and I2C scan.
2. Press the ignition button to start the simulated engine.
3. Turn the potentiometer to change throttle. RPM and speed update on the display.
4. Hold the brake button to reduce speed while ignition is on.
5. Toggle the headlights and observe the indicator and serial output.
6. Turn ignition off; simulated RPM and speed reset to zero.

Example serial output format:

```text
Throttle: 50.0%, RPM: 3400, Speed: 42.0 km/h, Brake: OFF, Headlights: ON
```

Values are simulated, not measurements from a vehicle. Current model constants include 800 idle RPM, 6000 maximum RPM, and a 180 km/h maximum target speed. Coolant temperature and battery voltage exist in the state model but are not currently simulated or included in periodic telemetry.

## Code layout

- `firmware/src/main.cpp`: input sampling, simulation, display, and serial telemetry.
- `firmware/src/DebouncedButton.cpp`: button debouncing.
- `firmware/include/DebouncedButton.h`: button interface.
- `firmware/include/VehicleState.h`: vehicle state model.
- `firmware/platformio.ini`: build and dependency configuration.

## Validation and status

Use `pio run` to check compilation and the manual sequence above to exercise the hardware. The test directory currently contains a placeholder README, not an automated simulation test suite. This project is intended for learning embedded state management and telemetry before adding further connectivity.
