# Ground Station Firmware

This directory contains the MKR WAN 1310 firmware that backs the physical MySat ground station bridge.

The ground-station firmware is built from this directory using its local `platformio.ini`.

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

- `include/config.h` - serial, LoRa, and retry tuning constants for the bridge
- `include/commands.h` - host-local `GROUND` command handling interface
- `include/link.h` - pending-command tracking, host forwarding, and duplicate-suppression helpers
- `include/protocol.h` - command token parsing and response-kind matching helpers
- `include/radio.h` - LoRa radio bring-up and packet send/receive interface
- `include/rf_envelope.h` - shared RF packet envelope and CRC helpers
- `include/sender.h` - host-local `GROUND` `ACK`, `ERR`, and `TLM` emission helpers
- `include/clock.h` - local clock state, sync policy helpers, and ISO timestamp formatting
- `include/led.h` - built-in LED activity pulse helper

### Source Files

- `src/main.cpp` - top-level setup, loop, and bridge orchestration
- `src/commands.cpp` - host-local `GROUND` command handling
- `src/link.cpp` - pending-command lifecycle, payload forwarding, and duplicate suppression
- `src/clock.cpp` - local clock state, satellite time sync, and ISO formatting
- `src/led.cpp` - built-in LED pulse behavior for RF activity
- `src/protocol.cpp` - command parsing and pending-response classification helpers
- `src/radio.cpp` - LoRa radio initialization and packet transport
- `src/rf_envelope.cpp` - RF packet encode/decode helpers and CRC-16/CCITT-FALSE implementation
- `src/sender.cpp` - host-local `GROUND` message emission

## Responsibilities

The current bridge firmware accepts host serial commands, wraps them in the shared RF envelope, forwards them over LoRa, reconstructs host-visible timestamps from the RF packet header, and sends received satellite payload lines back to the host over USB serial.

It also:

- owns retry and timeout behaviour for satellite-bound commands
- exposes the host-local `GROUND` target at the USB boundary
- emits a ground-station heartbeat and optional local status snapshot
- can seed its local clock from trusted satellite packet timestamps while still `UNSYNC`

## Documentation Boundaries

- generic protocol rules live in [../documentation/Protocol.md](../documentation/Protocol.md)
- generic telemetry rules live in [../documentation/Telemetry.md](../documentation/Telemetry.md)
- current RF link behaviour lives in [../documentation/RF.md](../documentation/RF.md)
- `GROUND` target behaviour lives in [../documentation/targets/GROUND.md](../documentation/targets/GROUND.md)
- this README stays focused on bridge build and source layout
