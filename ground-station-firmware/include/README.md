# Ground Station Firmware

This directory contains the Arduino Mega 2560 firmware that will back the physical MySat ground station hardware.

Current structure:

- `src/` - PlatformIO source files for the Mega 2560
- `include/` - project headers for the Mega 2560 firmware

The initial scaffold provides a minimal serial heartbeat sketch so the `mega2560` PlatformIO environment builds cleanly.
