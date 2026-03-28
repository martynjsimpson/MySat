# MySat

MySat is a CubeSat-style embedded project built around two collaborating parts:

- `satellite-firmware/` - firmware for an Arduino MKR WiFi 1010
- `ground-station-dashboard/` - a Node-RED ground-station dashboard

The project currently uses a structured serial protocol first, with the intent that the same logical message model can later be carried over RF.

## Repository layout

- `satellite-firmware/` - firmware source, headers, and satellite-specific documentation
- `ground-station-dashboard/` - Node-RED flow, local runtime settings, and dashboard documentation
- `ground-station-firmware/` - firmware source for the Arduino Mega 2560 ground station
- `documentation/` - protocol and telemetry specifications shared by both sides
- `platformio.ini` - root PlatformIO config for both firmware targets

## Documentation map

- [satellite-firmware/README.md](./satellite-firmware/README.md) - MKR WiFi 1010 firmware build, structure, and subsystem notes
- [ground-station-dashboard/README.md](./ground-station-dashboard/README.md) - Node-RED setup and runtime notes
- [documentation/README.md](./documentation/README.md) - documentation index and target reference map
- [documentation/Protocol.md](./documentation/Protocol.md) - generic command and response protocol
- [documentation/Telemetry.md](./documentation/Telemetry.md) - generic telemetry framing and snapshot rules
- [documentation/targets/](./documentation/targets) - target-specific command and telemetry reference pages

## Current direction

- The firmware remains the system of record for protocol behavior.
- The dashboard is now a source-controlled Node-RED project under `ground-station-dashboard/`.
- The hardware ground station firmware now lives under `ground-station-firmware/`.
- Shared wire-format rules should live in `documentation/`, not be repeated in multiple READMEs.
- Target-specific commands and fields should be documented on their own pages under `documentation/targets/`.
