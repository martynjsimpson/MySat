# Documentation

This directory contains the shared documentation for the firmware and ground station.

## Core Documents

- [Architecture.md](./Architecture.md) defines the repo-level system boundaries and project layout.
- [Protocol.md](./Protocol.md) defines the generic command and response protocol.
- [Telemetry.md](./Telemetry.md) defines generic telemetry framing and snapshot behavior.
- [RF.md](./RF.md) defines the current implemented RF architecture and link behaviour.
- [RF_Transition.md](./RF_Transition.md) tracks unfinished RF validation, open questions, and deferred future RF work.
- [OTA_Proposal.md](./OTA_Proposal.md) captures the separate discussion around remote firmware update and why LoRa OTA is not part of the baseline RF plan.
- [EPS_Proposal.md](./EPS_Proposal.md) captures the early concept for a monitored external subsystem power rail using an INA219.

## Target Reference

Each implemented target has its own reference page with the same structure:

- overview
- implemented commands
- telemetry fields
- behavior notes

Implemented target pages:

- [GROUND](./targets/GROUND.md)
- [TELEMETRY target](./targets/TELEMETRY_TARGET.md)
- [BATTERY](./targets/BATTERY.md)
- [GPS](./targets/GPS.md)
- [RTC](./targets/RTC.md)
- [THERMAL](./targets/THERMAL.md)
- [IMU](./targets/IMU.md)
- [ADCS](./targets/ADCS.md)
- [STATUS](./targets/STATUS.md)

## Boundaries

- generic protocol rules belong in `Protocol.md`
- generic telemetry rules belong in `Telemetry.md`
- target-specific command and field reference belongs in `documentation/targets/`
- firmware build and source layout belong in [../satellite-firmware/README.md](../satellite-firmware/README.md)
- ground-station firmware build layout belongs in [../ground-station-firmware/platformio.ini](../ground-station-firmware/platformio.ini)
- dashboard setup belongs in [../ground-station-dashboard/README.md](../ground-station-dashboard/README.md)
