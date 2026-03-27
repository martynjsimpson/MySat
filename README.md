# MySat

MySat is a CubeSat-style embedded project built around two collaborating parts:

- `satellite/` - firmware for an Arduino MKR WiFi 1010
- `ground-station/` - a Node-RED ground-station prototype

The project currently uses a structured serial protocol first, with the intent that the same logical message model can later be carried over RF.

## Repository layout

- `satellite/` - firmware source, headers, and satellite-specific documentation
- `ground-station/` - Node-RED flow, local runtime settings, and ground-station documentation
- `documentation/` - protocol and telemetry specifications shared by both sides
- `platformio.ini` - root PlatformIO config pointing at `satellite/src` and `satellite/include`

## Documentation map

- [satellite/README.md](./satellite/README.md) - firmware build, structure, and subsystem notes
- [ground-station/README.md](./ground-station/README.md) - Node-RED setup and runtime notes
- [documentation/README.md](./documentation/README.md) - documentation index and ownership
- [documentation/Protocol.md](./documentation/Protocol.md) - command and response protocol
- [documentation/Telemetry.md](./documentation/Telemetry.md) - telemetry snapshot and decoding rules

## Current direction

- The firmware remains the system of record for protocol behavior.
- The ground station is now a source-controlled Node-RED project under `ground-station/`.
- Shared wire-format rules should live in `documentation/`, not be repeated in multiple READMEs.
