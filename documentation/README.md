# Documentation

This directory contains the shared documentation for the firmware and ground station.

## Core Documents

- [Architecture.md](./Architecture.md) defines the repo-level system boundaries and project layout.
- [Protocol.md](./Protocol.md) defines the generic command and response protocol.
- [Telemetry.md](./Telemetry.md) defines generic telemetry framing and snapshot behavior.

## Target Reference

Each implemented target has its own reference page with the same structure:

- overview
- implemented commands
- telemetry fields
- behavior notes

Implemented target pages:

- [LED](./targets/LED.md)
- [TELEMETRY target](./targets/TELEMETRY_TARGET.md)
- [BATTERY](./targets/BATTERY.md)
- [GPS](./targets/GPS.md)
- [RTC](./targets/RTC.md)
- [THERMAL](./targets/THERMAL.md)
- [STATUS](./targets/STATUS.md)

## Boundaries

- generic protocol rules belong in `Protocol.md`
- generic telemetry rules belong in `Telemetry.md`
- target-specific command and field reference belongs in `documentation/targets/`
- firmware build and source layout belong in [../satellite-firmware/README.md](../satellite-firmware/README.md)
- ground-station firmware build layout belongs in [../ground-station-firmware/platformio.ini](../ground-station-firmware/platformio.ini)
- dashboard setup belongs in [../ground-station-dashboard/README.md](../ground-station-dashboard/README.md)
