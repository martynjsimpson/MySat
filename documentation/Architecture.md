# Architecture

This repository contains one system split across three runtime parts:

- `satellite-firmware/` runs on the flight-side Arduino board and is the source of truth for command execution, telemetry production, and device control.
- `ground-station-firmware/` runs on the physical ground-station Arduino board and handles the host USB boundary, LoRa bridge transport, retry ownership, and host-local ground-station control.
- `ground-station-dashboard/` runs on a host machine as a Node-RED-backed operator interface, with the current bespoke frontend implemented through `uibuilder`.

## System Boundaries

- Shared message rules belong in [Protocol.md](./Protocol.md) and [Telemetry.md](./Telemetry.md).
- Shared RF transport behaviour belongs in [RF.md](./RF.md).
- Target-specific behavior belongs in [targets/](./targets).
- Board-specific build and source layout belongs in each firmware project's local README or PlatformIO config.
- Dashboard runtime and UI behavior belongs in [../ground-station-dashboard/README.md](../ground-station-dashboard/README.md).

## PlatformIO Layout

The firmware projects intentionally use separate PlatformIO entry points:

- [../platformio.ini](../platformio.ini) builds `satellite-firmware/` for the `mkrwan1310`
- [../ground-station-firmware/platformio.ini](../ground-station-firmware/platformio.ini) builds `ground-station-firmware/` for the `mkrwan1310`

This split is deliberate. A combined multi-environment root PlatformIO setup built successfully, but it destabilized VS Code IntelliSense enough to make day-to-day work noisy and unreliable.

## Current Message Flow

Today the system is RF-backed while preserving the same logical protocol at the host boundary:

1. The dashboard or host console emits newline-terminated protocol commands over USB serial to the ground station.
2. The ground station either handles host-local `GROUND` commands itself or wraps satellite-bound commands in the RF envelope and transmits them over LoRa.
3. The satellite firmware decodes the RF envelope, parses the logical command, executes target behavior, and emits `ACK`, `ERR`, and `TLM` responses.
4. The ground station forwards satellite responses back to the host unchanged and may also emit its own protocol-shaped `GROUND` `ACK`, `ERR`, and `TLM` messages.
5. The dashboard stores and presents both satellite and ground-station state to the operator.

The satellite firmware transport abstraction is what made that link swap possible without rewriting the higher-level command and telemetry flow.

In the current firmware this split looks like:

- `transport.h` defines the transport-facing byte and print operations
- `transport_lora.cpp` implements those operations using LoRa and the shared RF envelope
- `protocol.cpp` consumes the transport for inbound command bytes
- `sender.cpp` consumes the transport for outbound protocol messages
- `protocol_dispatch.cpp` uses the transport flush path before reset

## Working Agreement

When changes span more than one runtime part:

- update shared protocol documentation first or alongside code
- treat satellite firmware, ground-station firmware, and dashboard as one coordinated system
- avoid duplicating wire-format rules in multiple READMEs
- keep board-specific toolchain choices local to the relevant project
