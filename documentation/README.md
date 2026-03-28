# Documentation

This directory contains the shared documentation for the firmware and ground station.

## Core Documents

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
- [STATUS](./targets/STATUS.md)

## Boundaries

- generic protocol rules belong in `Protocol.md`
- generic telemetry rules belong in `Telemetry.md`
- target-specific command and field reference belongs in `documentation/targets/`
- firmware build and source layout belong in [../satellite/README.md](../satellite/README.md)
- ground-station setup belongs in [../ground-station/README.md](../ground-station/README.md)
