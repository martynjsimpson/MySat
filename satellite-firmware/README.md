# Satellite Firmware

This directory contains the MySat firmware for the Arduino MKR WAN 1310.

The firmware is built with PlatformIO from the repository root using [platformio.ini](../platformio.ini). The Arduino Mega 2560 ground-station firmware is built from its own separate project file at [../ground-station-firmware/platformio.ini](../ground-station-firmware/platformio.ini).

## Build and Run

Build:

```bash
platformio run --environment mkrwan1310
```

Upload:

```bash
platformio run --target upload --environment mkrwan1310
```

Serial monitor:

```bash
platformio device monitor --environment mkrwan1310
```

## Source Layout

### Public Headers

- `include/commands.h` - protocol enums and parsed `Command` model
- `include/config.h` - central defaults and tunable parameters
- `include/gps.h` - GPS subsystem interface
- `include/led.h` - built-in LED activity indicator interface
- `include/pmic.h` - battery subsystem interface
- `include/imu.h` - MPU-6050 and QMC5883L IMU subsystem interface
- `include/adcs.h` - derived attitude subsystem interface
- `include/protocol.h` - inbound command buffering and line-framing interface
- `include/rf_envelope.h` - RF packet envelope and CRC helpers for the planned LoRa transport
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
- `src/rf_envelope.cpp` - RF packet encode/decode helpers and CRC-16/CCITT-FALSE implementation
- `src/telemetry_config.cpp` - telemetry flags, interval, and telemetry control target
- `src/telemetry_scheduler.cpp` - periodic snapshot scheduling
- `src/led.cpp` - built-in LED activity pulse handling for transport traffic
- `src/pmic.cpp` - PMIC setup and battery reporting
- `src/gps.cpp` - GPS polling, state, and control
- `src/gps_report.cpp` - GPS `GET` handling and reporting
- `src/rtc.cpp` - RTC `GET` and `SET` handling
- `src/rtc_core.cpp` - RTC storage, formatting, and time conversion
- `src/rtc_sync.cpp` - RTC GPS sync behavior
- `src/status.cpp` - startup and heartbeat reporting
- `src/thermal.cpp` - DHT11 polling, caching, and reporting
- `src/imu.cpp` - MPU-6050 and QMC5883L polling, caching, and reporting
- `src/adcs.cpp` - IMU-derived roll, pitch, heading, source, and yaw-rate reporting
- `src/sender.cpp` - wire-format message emission
- `src/transport_lora.cpp` - LoRa-backed transport that carries one logical protocol line per RF packet

## Current Targets

- `TELEMETRY` - telemetry scheduler control and reporting policy
- `BATTERY` - PMIC-backed battery reporting and charger enable control
- `GPS` - GPS control and position reporting
- `RTC` - RTC time, synchronisation state, and clock source reporting
- `THERMAL` - DHT11 temperature and humidity reporting
- `IMU` - MPU-6050 motion and QMC5883L magnetic field reporting
- `ADCS` - IMU-derived attitude, heading, and yaw-rate reporting
- `STATUS` - startup event and non-disableable heartbeat

The current GPS implementation is configured for the MKR GPS connected over the I2C cable path.

The current thermal implementation is configured for a DHT11 on `D7`, with temperature and humidity reported through the `THERMAL` target.

The current IMU implementation is configured for an MPU-6050 and QMC5883L on the shared I2C bus, with acceleration, gyroscope, magnetic field, and heading data reported through the `IMU` target.

The current ADCS implementation is derived from the `IMU` target and reports roll, pitch, tilt-compensated heading, and yaw-rate estimates through the `ADCS` target.

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

The active transport implementation is now `src/transport_lora.cpp`, which wraps one logical protocol line per RF packet using the shared RF envelope and feeds decoded payloads back into the existing parser path. The built-in LED is now used as a short activity pulse for RF send/receive events rather than as a commandable subsystem. The point of the abstraction remains the same: parser, dispatcher, sender, and telemetry code stay transport-agnostic while the link layer evolves underneath them.
