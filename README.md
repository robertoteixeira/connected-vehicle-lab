# Connected Vehicle Lab

A connected-device portfolio project built with ESP32 firmware and **Vehicle Lab**, a native SwiftUI and CoreBluetooth iOS app. Physical controls drive a simulated vehicle; its state appears on an OLED and streams to the app over BLE. The app also reads device identity and configures the ESP32's Wi-Fi connection.

Values are simulated. This project does not interface with a real vehicle or CAN bus. Wi-Fi provisioning works locally; backend and Azure IoT integration are planned, not implemented.

## System overview

```text
Buttons and throttle potentiometer
                |
                v
ESP32: inputs and vehicle simulation ---> SSD1306 OLED
                |
                v
         BLE GATT server <------------> Vehicle Lab on iPhone
                |                       - Vehicle telemetry
                |                       - Device identity
                |                       - Wi-Fi provisioning and results
                v
     Wi-Fi connection attempt
     (queued request, 15-second deadline)
```

BLE carries the current app-to-device communication. Joining Wi-Fi does not yet transmit telemetry to a backend. Serial output provides startup, input-change, and connectivity diagnostics; periodic serial vehicle telemetry is currently disabled.

## Firmware

C++ firmware built with the Arduino framework and PlatformIO for an ESP32 `esp32dev` board.

- Debounced ignition, brake, and headlight buttons.
- Sixteen-sample averaging and a deadband for the analog throttle input.
- Throttle-based RPM and target speed; elapsed-time acceleration, coasting deceleration, and braking affect speed.
- A 128×64 SSD1306 OLED refreshed every 100 ms, plus an I2C scan at startup.
- Vehicle telemetry checked every 500 ms, with BLE notifications only when the serialized payload changes.
- BLE services for vehicle state, device identity, Wi-Fi credentials, and provisioning status.
- Wi-Fi requests passed from the BLE callback to the main loop through a one-slot FreeRTOS queue. A newer request replaces a queued request; connection attempts run in the main loop rather than the BLE write callback.
- Explicit provisioning states and a 15-second Wi-Fi attempt deadline.
- BLE advertising restarted after disconnection.

The simulation uses 800 idle RPM, 6000 maximum RPM, and a maximum target speed of 180 km/h. Turning ignition off resets speed and RPM to zero; throttle and headlight state are retained.

### Hardware and wiring

| Component | ESP32 connection |
| --- | --- |
| Ignition button | GPIO 27 |
| Brake button | GPIO 26 |
| Headlight button | GPIO 25 |
| Throttle potentiometer wiper | GPIO 34 |
| OLED SDA | GPIO 21 |
| OLED SCL | GPIO 22 |
| OLED I2C address | `0x3C` |

Use a common ground. Buttons use pull-up inputs, so connect each button between its GPIO and GND; a press reads LOW. Connect the potentiometer's outer terminals to 3.3 V and GND, keeping its output in the ESP32's 3.3 V range. Power the OLED according to its module specifications.

### Build and run

Install PlatformIO Core or its VS Code extension:

```sh
git clone https://github.com/robertoteixeira/connected-vehicle-lab.git
cd connected-vehicle-lab/firmware
pio run
pio run --target upload
pio device monitor --baud 115200
```

Adafruit GFX and Adafruit SSD1306 dependencies are declared in `firmware/platformio.ini`. Specify the upload/monitor port if automatic detection fails.

## iOS app — Vehicle Lab

A native **SwiftUI + CoreBluetooth** app targeting **iOS 17.6 and later**.

- Scans for the vehicle service UUID and automatically connects to the first discovered matching device.
- Reads device identity and shows vehicle readings after the first valid telemetry payload arrives.
- Explains Bluetooth availability, including disabled Bluetooth and denied access.
- Cancels pending BLE connection attempts after 15 seconds and schedules a scan retry after 2 seconds on failure or disconnection. Recovery is intended for foreground use.
- Rejects malformed or duplicate telemetry and identity fields, invalid booleans, non-finite numbers, and out-of-range telemetry values.
- Provides a manual Wi-Fi form with shared validation for UTF-8 byte lengths, separator compatibility, and embedded null characters.
- Distinguishes a BLE write acknowledgment from the ESP32's Wi-Fi provisioning result.
- Keeps provisioning pending until a result, write error, or disconnection. If no result arrives within 25 seconds after acknowledgment, displays a warning while retaining the pending state.
- Uses main-actor-isolated observable state and cancellable tasks for connection and provisioning-result deadlines. Telemetry and provisioning have dedicated SwiftUI views.

Wi-Fi status describes the last provisioning attempt. A successful attempt is not continuous monitoring of the device's Wi-Fi connection.

### Build and run

1. Open `ios/ConnectedVehicle/ConnectedVehicle.xcodeproj` in Xcode.
2. Select the `ConnectedVehicle` app target and choose your development team under **Signing & Capabilities** for device installation.
3. Select a physical iPhone running iOS 17.6 or later and the `ConnectedVehicle` scheme.
4. Build and run with **⌘R**, then allow Bluetooth access when prompted.
5. Keep the ESP32 powered nearby. The app discovers the board automatically; no manual selection in iPhone Bluetooth Settings is required.

Use a physical iPhone to validate BLE communication with the ESP32. The simulator can run model tests and SwiftUI previews without the board.

### Tests

`ConnectedVehicleTests.swift` contains **17 XCTest test methods** covering telemetry parsing, device identity parsing, malformed and duplicate fields, boolean and numeric validation, and Wi-Fi credential validation, including UTF-8 byte limits and null-character rejection.

In Xcode, select an iPhone simulator, open the Test Navigator (**⌘6**), and run the `ConnectedVehicleTests` target. **⌘U** runs the scheme's selected tests, which also include the existing UI test target. The UI tests are still launch/performance templates, not a BLE integration suite.

The 17 unit tests have been verified on an iPhone 16 simulator running iOS 18.4. BLE behavior is validated separately on real hardware.

## BLE protocol

The ESP32 advertises as `ConnectedVehicle`. The app display name is `Vehicle Lab`.

| Characteristic | Properties | Example payload |
| --- | --- | --- |
| Vehicle state | Read, Notify | `ign=1,speed=42.5,rpm=2500,thr=30.0,brk=0,lgt=1` |
| Device identity | Read | `id=VEH-001,hw=ESP32-DEVKIT-V1,fw=0.1.0` |
| Wi-Fi configuration | Write | `ssid=Lab Network,password=example-password` |
| Wi-Fi provisioning status | Read, Notify | `idle`, `connecting`, `connected`, `timed_out` |

See [BLE protocol](docs/BLE_PROTOCOL.md) for UUIDs, payload rules, notification behavior, queue semantics, and limitations.

## Try it on hardware

1. Flash the firmware and open the serial monitor. Check startup messages and the OLED's I2C address.
2. Launch Vehicle Lab on your iPhone and confirm the connection and device identity appear.
3. Turn ignition on and adjust the potentiometer. Compare RPM and speed on the OLED and in the app.
4. Hold the brake to reduce speed, then toggle the headlights.
5. Turn ignition off: speed and RPM become zero while throttle and headlight state remain unchanged.
6. Submit Wi-Fi credentials in the app. Observe `connecting`, followed by success or a timeout. An acknowledged BLE write alone does not mean Wi-Fi connected.
7. With the app active in the foreground, power-cycle the ESP32. Confirm the app reconnects and resumes telemetry when the board returns.

## Code layout

```text
firmware/
  platformio.ini                     Board, framework, and dependencies
  src/main.cpp                       Inputs, simulation, OLED, diagnostics, BLE server setup
  src/BleVehicleService.cpp           Vehicle state and change-only notifications
  src/BleProvisioningService.cpp      Identity, Wi-Fi configuration, and provisioning status
  src/WifiManager.cpp                 Request queue, provisioning states, and deadline
  src/DebouncedButton.cpp             Button debouncing
  include/                           State models and component interfaces

ios/ConnectedVehicle/
  ConnectedVehicle.xcodeproj/         Xcode project and shared scheme
  ConnectedVehicle/
    App/ConnectedVehicleApp.swift     App entry point and manager ownership
    Bluetooth/BluetoothManager.swift  CoreBluetooth lifecycle and provisioning coordination
    Models/                          Payload parsing, state, identity, and credentials
    Views/ContentView.swift           Dashboard composition and connection messages
    Views/VehicleTelemetryView.swift  Vehicle readings
    Views/WifiProvisioningView.swift  Credential entry and provisioning feedback
  ConnectedVehicleTests/             Model and validation unit tests
  ConnectedVehicleUITests/           Template launch and performance tests

docs/BLE_PROTOCOL.md                  BLE protocol reference
```

## Status and next steps

The ESP32 and iOS app work together on real hardware. Current limitations include:

- No protocol version or request identifiers.
- Invalid credential payloads are rejected through serial logging, without an application-level rejection response.
- BLE pairing and encrypted access are not configured by the firmware.
- Device identity uses a hardcoded prototype ID, and the app has no device-selection screen.
- No implemented background BLE workflow or continuous Wi-Fi connection-loss reporting.
- No automated firmware unit tests or BLE integration tests yet.

Development proceeds in this order:

1. **Finish the iOS experience and device interaction:** clearer connection feedback, provisioning rejection responses, asynchronous recovery, device selection, access protection, UI polish, and verification.
2. **Build the backend and Azure IoT integration:** device authentication, cloud telemetry ingestion, processing, storage, and APIs. Concrete service choices will be made during that phase.
3. **Explore additional mobile implementations:** native Android, Flutter, and React Native.
4. **Extract SDKs:** identify reusable protocol, validation, and communication components from the completed implementations.
