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
- `include/protocol.h` - serial command reader interface
- `include/rtc.h` - RTC interface
- `include/sender.h` - outbound `ACK`, `ERR`, and `TLM` helpers
- `include/status.h` - status heartbeat interface
- `include/telemetry.h` - telemetry control and scheduler interface

### Source Files

- `src/main.cpp` - top-level `setup()` and `loop()`
- `src/protocol.cpp` - serial command buffering and line framing
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
- `src/sender.cpp` - wire-format message emission

## Current Targets

- `LED` - built-in LED control and status
- `TELEMETRY` - telemetry scheduler control and reporting policy
- `BATTERY` - PMIC-backed battery reporting
- `GPS` - GPS control and position reporting
- `RTC` - RTC time and synchronisation state
- `STATUS` - startup event and non-disableable heartbeat

The current GPS implementation is configured for the MKR GPS connected over the I2C cable path.

## Documentation Boundaries

- Generic wire protocol rules live in [../documentation/Protocol.md](../documentation/Protocol.md).
- Generic telemetry rules live in [../documentation/Telemetry.md](../documentation/Telemetry.md).
- Target-specific command and telemetry reference lives in [../documentation/targets/](../documentation/targets).
- This README stays focused on firmware structure and build workflow.
