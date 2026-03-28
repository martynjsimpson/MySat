# Architecture

This repository contains one system split across three runtime parts:

- `satellite-firmware/` runs on the flight-side Arduino board and is the source of truth for command execution, telemetry production, and device control.
- `ground-station-firmware/` runs on the physical ground-station Arduino board and is intended to handle local hardware and link transport responsibilities.
- `ground-station-dashboard/` runs on a host machine as a Node-RED dashboard for operator interaction, state display, and command issuance.

## System Boundaries

- Shared message rules belong in [Protocol.md](./Protocol.md) and [Telemetry.md](./Telemetry.md).
- Target-specific behavior belongs in [targets/](./targets).
- Board-specific build and source layout belongs in each firmware project's local README or PlatformIO config.
- Dashboard runtime and UI behavior belongs in [../ground-station-dashboard/README.md](../ground-station-dashboard/README.md).

## PlatformIO Layout

The firmware projects intentionally use separate PlatformIO entry points:

- [../platformio.ini](../platformio.ini) builds `satellite-firmware/` for the `mkrwifi1010`
- [../ground-station-firmware/platformio.ini](../ground-station-firmware/platformio.ini) builds `ground-station-firmware/` for the `mega2560`

This split is deliberate. A combined multi-environment root PlatformIO setup built successfully, but it destabilized VS Code IntelliSense enough to make day-to-day work noisy and unreliable.

## Current Message Flow

Today the system is serial-first:

1. The dashboard emits commands using the documented wire format.
2. The satellite firmware parses commands, executes target behavior, and emits `ACK`, `ERR`, and `TLM` responses.
3. The dashboard stores and presents those responses to the operator.

The satellite firmware now has a transport abstraction in place so the current serial transport can later be replaced with a radio-backed transport without rewriting the higher-level command and telemetry flow.

## Working Agreement

When changes span more than one runtime part:

- update shared protocol documentation first or alongside code
- treat satellite firmware, ground-station firmware, and dashboard as one coordinated system
- avoid duplicating wire-format rules in multiple READMEs
- keep board-specific toolchain choices local to the relevant project
