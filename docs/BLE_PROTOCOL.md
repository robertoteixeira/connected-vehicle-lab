# BLE protocol

The ESP32 advertises as `ConnectedVehicle`. Clients should discover it
using the vehicle service UUID rather than relying on its name.

Payloads use UTF-8 text. No terminating null byte is transmitted.

The iOS client lists matching peripherals for the user to select. It
saves the selected CoreBluetooth peripheral UUID in `UserDefaults` and
reconnects when that peripheral is rediscovered, including after app
relaunch. This identifier is distinct from the firmware's `VEH-…` ID,
which is read after connection. A saved selection does not authenticate
the device, and the client does not persist Wi-Fi credentials in
`UserDefaults`.

The iOS **Choose another vehicle** action clears the saved selection and
cancels the active or pending local connection. After the cancellation
callback, the client resumes scanning and waits for a new selection.

## Services and characteristics

| Purpose | UUID | Properties |
| --- | --- | --- |
| Vehicle service | `7b9f0001-6c5a-4e8f-9d3a-2f1b7c8e0001` | — |
| Vehicle state | `7b9f0002-6c5a-4e8f-9d3a-2f1b7c8e0001` | Read, Notify |
| Provisioning service | `7b9f0010-6c5a-4e8f-9d3a-2f1b7c8e0001` | — |
| Device identity | `7b9f0011-6c5a-4e8f-9d3a-2f1b7c8e0001` | Read |
| Wi-Fi configuration | `7b9f0012-6c5a-4e8f-9d3a-2f1b7c8e0001` | Write |
| Wi-Fi provisioning status | `7b9f0013-6c5a-4e8f-9d3a-2f1b7c8e0001` | Read, Notify |
| Wi-Fi request result | `7b9f0014-6c5a-4e8f-9d3a-2f1b7c8e0001` | Read, Notify |

## Vehicle state

Example:

```text
ign=1,speed=42.5,rpm=2500,thr=30.0,brk=0,lgt=1
```

| Field | Meaning | Current simulator range |
| --- | --- | --- |
| `ign` | Ignition enabled | `0` or `1` |
| `speed` | Speed in km/h | 0–180 |
| `rpm` | Engine RPM | 0–6000 |
| `thr` | Throttle percentage | 0–100 |
| `brk` | Brake pressed | `0` or `1` |
| `lgt` | Headlights enabled | `0` or `1` |

All fields are required. Numeric values must be finite. Field order is
not significant. Clients must reject duplicate keys and malformed fields.
Unknown nonempty fields with valid structure may be ignored.

The firmware checks for changes every 500 ms and notifies only when
the serialized payload changes. Silence alone does not indicate a
broken connection. Read the characteristic for an initial snapshot.

## Device identity

Example:

```text
id=VEH-A1B2C3D4E5F6,hw=ESP32-DEVKIT-V1,fw=0.1.0
```

`id`, `hw`, and `fw` are required and nonempty. Field order is not
significant. Clients must reject duplicate keys and malformed fields.
Unknown nonempty fields with valid structure may be ignored.

The device ID is generated from the ESP32's factory MAC value and uses
the format `VEH-` followed by 12 uppercase hexadecimal characters. It
remains stable across restarts and firmware uploads. It identifies a
board but does not authenticate it. The hexadecimal suffix is the
formatted value returned by `ESP.getEfuseMac()`, not a conventional
colon-separated MAC address.

## Wi-Fi configuration

Write a complete payload with response:

```text
ssid=Lab Network,password=example-password
```

Rules:

- The payload begins with `ssid=`.
- The first `,password=` separates the SSID from the password.
- SSID length is 1–32 UTF-8 bytes.
- Password length is 0–64 UTF-8 bytes.
- An empty password is allowed for open networks.
- Neither value may contain a null byte.
- The SSID cannot contain the separator `,password=`.
- Spaces and capitalization are preserved.
- Clients must respect the connection's maximum write length.

A successful BLE write acknowledgment confirms the transport write,
not credential acceptance or Wi-Fi connection success.

Malformed requests are rejected without starting a connection attempt.
The Wi-Fi request result characteristic reports rejection, including
when the manager cannot accept a request into its queue.

The firmware holds one queued request. A newer request replaces any
request that has not yet been consumed by the main loop. A request
received during an active attempt starts a new attempt and resets its
deadline. Clients should wait for the current attempt's result before
submitting another request.

## Wi-Fi request result

| Payload | Meaning |
| --- | --- |
| `idle` | No request has been processed since boot |
| `accepted` | The most recent request was accepted into the queue |
| `rejected` | Parsing, validation, or queue submission failed |

Subscribe before submitting credentials. The firmware sends a notification
for every processed write, including repeated identical results. A read
returns the latest stored result, which may belong to a previous request;
clients must not use that snapshot as the outcome of a new request.

Acceptance does not confirm the SSID exists or the password is correct.
Use provisioning status for the subsequent connection outcome. Request
results, provisioning status, and BLE write acknowledgments can arrive
in different orders. A later write acknowledgment must not overwrite
a rejection already received for the pending request.

Results have no request identifiers or detailed rejection reason codes.
The current protocol assumes clients coordinate one request at a time;
it does not isolate requests made by multiple clients.

## Wi-Fi provisioning status

Subscribe to notifications, then read the current value.

| Payload | Meaning |
| --- | --- |
| `idle` | No attempt has started since boot |
| `connecting` | A connection attempt is running |
| `connected` | The last attempt succeeded |
| `timed_out` | The last attempt exceeded its 15-second deadline |

Notifications are sent when the state changes. Reading returns the
latest stored state, including after BLE reconnection. Clients should
tolerate repeated values and handle unrecognized values without crashing.

These states describe provisioning attempts. The firmware does not
currently report a later loss of Wi-Fi connectivity.

## Current limitations

- No protocol version or request identifiers.
- Request rejections do not include detailed reason codes or request correlation.
- No BLE pairing or encrypted access configured by this firmware.
- Credentials are not persisted by application code; Wi-Fi stack
  persistence behavior is not defined by this protocol.
