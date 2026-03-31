# Ground Station Firmware

This directory contains the MKR WAN 1310 firmware that backs the physical MySat ground station bridge.

Current structure:

- `src/` - PlatformIO source files for the MKR WAN 1310 bridge firmware
- `include/` - project headers shared by the bridge firmware
- `include/config.h` - serial, LoRa, and retry tuning constants for the bridge

The current bridge firmware accepts host serial commands, wraps them in the shared RF envelope, forwards them over LoRa, reconstructs host-visible timestamps from the RF packet header, and sends received satellite payload lines back to the host over USB serial.
