# Satellite Firmware

This directory contains the MySat firmware for the Arduino MKR WiFi 1010.

The firmware is built with PlatformIO from the repository root using [platformio.ini](../platformio.ini). The Arduino Mega 2560 ground-station firmware is built from its own separate project file at [../ground-station-firmware/platformio.ini](../ground-station-firmware/platformio.ini).

## Build and Run

Build:

```bash
platformio run --environment mkrwifi1010
```

Upload:

```bash
platformio run --target upload --environment mkrwifi1010
```

Serial monitor:

```bash
platformio device monitor --environment mkrwifi1010
```

## Source Layout

### Public Headers

- `include/commands.h` - protocol enums and parsed `Command` model
- `include/config.h` - central defaults and tunable parameters
- `include/gps.h` - GPS subsystem interface
- `include/led.h` - LED subsystem interface
- `include/pmic.h` - battery subsystem interface
- `include/imu.h` - MPU-6050 IMU subsystem interface
- `include/protocol.h` - inbound command buffering and line-framing interface
- `include/rtc.h` - RTC interface
- `include/sender.h` - outbound `ACK`, `ERR`, and `TLM` helpers
- `include/status.h` - status heartbeat interface
- `include/thermal.h` - DHT11 thermal interface
- `include/telemetry.h` - telemetry control and scheduler interface
- `include/transport.h` - transport abstraction used by protocol and sender code

### Source Files

- `src/main.cpp` - top-level `setup()` and `loop()`
- `src/protocol.cpp` - inbound command buffering and newline framing
- `src/protocol_parser.cpp` - token parsing and `Command` construction
- `src/protocol_dispatch.cpp` - command execution and target dispatch
- `src/telemetry_config.cpp` - telemetry flags, interval, and telemetry control target
- `src/telemetry_scheduler.cpp` - periodic snapshot scheduling
- `src/led.cpp` - LED control and reporting
- `src/pmic.cpp` - PMIC setup and battery reporting
- `src/gps.cpp` - GPS polling, state, and control
- `src/gps_report.cpp` - GPS `GET` handling and reporting
- `src/rtc.cpp` - RTC `GET` and `SET` handling
- `src/rtc_core.cpp` - RTC storage, formatting, and time conversion
- `src/rtc_sync.cpp` - RTC GPS sync behavior
- `src/status.cpp` - startup and heartbeat reporting
- `src/thermal.cpp` - DHT11 polling, caching, and reporting
- `src/imu.cpp` - MPU-6050 polling, caching, and reporting
- `src/sender.cpp` - wire-format message emission
- `src/transport_serial.cpp` - current serial-backed transport implementation

## Current Targets

- `LED` - built-in LED control and status
- `TELEMETRY` - telemetry scheduler control and reporting policy
- `BATTERY` - PMIC-backed battery reporting
- `GPS` - GPS control and position reporting
- `RTC` - RTC time and synchronisation state
- `THERMAL` - DHT11 temperature and humidity reporting
- `IMU` - MPU-6050 acceleration and gyroscope reporting
- `STATUS` - startup event and non-disableable heartbeat

The current GPS implementation is configured for the MKR GPS connected over the I2C cable path.

The current thermal implementation is configured for a DHT11 on `D7`, with temperature and humidity reported through the `THERMAL` target.

The current IMU implementation is configured for an MPU-6050 on the shared I2C bus, with acceleration and gyroscope data reported through the `IMU` target.

## Documentation Boundaries

- Generic wire protocol rules live in [../documentation/Protocol.md](../documentation/Protocol.md).
- Generic telemetry rules live in [../documentation/Telemetry.md](../documentation/Telemetry.md).
- Target-specific command and telemetry reference lives in [../documentation/targets/](../documentation/targets).
- This README stays focused on firmware structure and build workflow.

## Transport Layer

The firmware is still serial-first today, but command and response I/O no longer call `Serial` directly throughout the codebase.

- `setupTransport()` is called early in `setup()` to initialize the active link.
- `protocol.cpp` reads inbound bytes through `transportAvailable()` and `transportRead()`.
- `sender.cpp` emits `ACK`, `ERR`, and `TLM` messages through `transportWrite(...)` helpers.
- `protocol_dispatch.cpp` uses `transportFlush()` before reset so the reboot acknowledgement has a chance to leave the device.

At the moment the transport implementation is `src/transport_serial.cpp`, which simply forwards these operations to `Serial`. The point of the abstraction is to let us swap the underlying link later, such as a radio transport, without rewriting the higher-level parser, dispatcher, sender, or telemetry logic.
