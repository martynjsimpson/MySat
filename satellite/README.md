# Satellite Firmware

This directory contains the MySat firmware for the Arduino MKR WiFi 1010.

The firmware is built with PlatformIO from the repository root. The root [platformio.ini](../platformio.ini) points PlatformIO at `satellite/src` and `satellite/include`.

## Build and run

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

## Source layout

- `include/commands.h` - protocol enums and the parsed `Command` model
- `include/protocol.h` - serial command reader interface
- `include/telemetry.h` - periodic telemetry control and snapshot interface
- `include/led.h` - LED subsystem interface
- `include/pmic.h` - battery / PMIC telemetry interface
- `include/rtc.h` - RTC and device clock interface
- `include/sender.h` - structured outbound message helpers
- `src/main.cpp` - top-level `setup()` and `loop()`
- `src/protocol.cpp` - command parsing and dispatch
- `src/telemetry.cpp` - telemetry settings and periodic snapshot scheduling
- `src/led.cpp` - LED control and LED telemetry
- `src/pmic.cpp` - PMIC setup and battery telemetry
- `src/rtc.cpp` - RTC setup, ISO time formatting, and clock sync handling
- `src/sender.cpp` - `ACK`, `ERR`, and `TLM` line emission

## Firmware architecture

The firmware currently follows a simple pattern:

1. Receive a newline-terminated serial command.
2. Parse `COMMAND,TARGET,PARAMETER,VALUE` into a typed `Command`.
3. Dispatch to a target-specific handler.
4. Emit structured `ACK`, `ERR`, or `TLM` lines with RTC-based ISO UTC timestamps.
5. Periodically emit telemetry snapshots when enabled.

## Current subsystems

- `LED` - controllable LED policy and state reporting
- `STATUS` - non-disableable heartbeat counter reported with periodic snapshots
- `TELEMETRY` - telemetry master enable/disable, interval control, and telemetry-status reporting control
- `BATTERY` - PMIC-backed battery telemetry reporting
- `RTC` - current time and clock synchronisation state

## Documentation boundaries

- Protocol definition belongs in [../documentation/Protocol.md](../documentation/Protocol.md).
- Telemetry semantics and decoding rules belong in [../documentation/Telemetry.md](../documentation/Telemetry.md).
- This README should stay focused on firmware structure and build workflow.
