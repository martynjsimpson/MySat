# Documentation

This directory contains the shared specifications used by both the firmware and the ground station.

## Document ownership

- [Protocol.md](./Protocol.md) owns the wire protocol:
  command format, response format, tokens, and message types.
- [Telemetry.md](./Telemetry.md) owns telemetry-specific behavior:
  telemetry snapshots, telemetry fields, and decoding guidance for a ground station.

## Documentation rules

- High-level repo navigation belongs in the root [README.md](../README.md).
- Firmware build and code structure belong in [../satellite/README.md](../satellite/README.md).
- Ground-station setup and runtime notes belong in [../ground-station/README.md](../ground-station/README.md).
- Shared protocol rules should be documented here once and referenced from elsewhere.
