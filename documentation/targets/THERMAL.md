# THERMAL Target

## Purpose

`THERMAL` reports temperature and humidity from a DHT11 connected to the satellite firmware and allows the thermal sensor service to be enabled or disabled.

The current firmware is written for a simple single-wire DHT11 module connected directly to the MKR WAN 1310.

## Supported GET Commands

| Command | Meaning |
|---|---|
| `GET,THERMAL,NONE,NONE` | Returns all implemented thermal telemetry fields |
| `GET,THERMAL,TELEMETRY,NONE` | Returns thermal periodic telemetry enable state |
| `GET,THERMAL,ENABLE,NONE` | Returns whether thermal sampling is enabled |
| `GET,THERMAL,AVAILABLE,NONE` | Returns whether a valid recent sensor read is available |
| `GET,THERMAL,TEMPERATURE_C,NONE` | Returns temperature in Celsius |
| `GET,THERMAL,HUMIDITY_P,NONE` | Returns relative humidity in percent |

## Supported SET Commands

| Command | Meaning |
|---|---|
| `SET,THERMAL,ENABLE,TRUE` | Enables thermal sampling |
| `SET,THERMAL,ENABLE,FALSE` | Disables thermal sampling and clears availability |
| `SET,THERMAL,TELEMETRY,ENABLE` | Includes thermal data in periodic telemetry |
| `SET,THERMAL,TELEMETRY,DISABLE` | Omits thermal data from periodic telemetry |

## Telemetry Fields

| Field | Meaning | Value form |
|---|---|---|
| `THERMAL,TELEMETRY` | Whether thermal is included in periodic telemetry | `TRUE`, `FALSE` |
| `THERMAL,ENABLE` | Whether thermal sampling is enabled | `TRUE`, `FALSE` |
| `THERMAL,AVAILABLE` | Whether the last read produced valid temperature and humidity | `TRUE`, `FALSE` |
| `THERMAL,TEMPERATURE_C` | Temperature in Celsius | float |
| `THERMAL,HUMIDITY_P` | Relative humidity percentage | float |

## Wiring

### Current Default Pin

The firmware currently expects the DHT11 data line on MKR digital pin `D7`.

This is configured in [config.h](../../satellite-firmware/include/config.h) as:

```cpp
constexpr uint8_t dataPin = 7;
```

### 3-Pin DHT11 Module

For the common 3-pin breakout/module version:

- `VCC` -> MKR `3.3V`
- `GND` -> MKR `GND`
- `DATA` -> MKR `D7`

These breakout boards typically already include the required pull-up resistor on the data line.

### Bare 4-Pin DHT11 Sensor

If you later use the bare sensor instead of the 3-pin module:

- `VCC` -> MKR `3.3V`
- `DATA` -> MKR `D7`
- add a `10k` pull-up resistor between `DATA` and `3.3V`
- `GND` -> MKR `GND`

## Notes

- Reads are rate-limited in firmware so the DHT11 is not polled faster than its supported cadence.
- The current firmware limits reads to once every `2000 ms`.
- Sampling is enabled by default in firmware.
- Thermal periodic telemetry is disabled by default until explicitly enabled.
- `THERMAL,AVAILABLE,FALSE` usually means the sensor is not wired, not yet ready, or the last read failed.
- When thermal sampling is disabled with `SET,THERMAL,ENABLE,FALSE`, availability is cleared immediately.
- `GET` works even when thermal periodic telemetry is disabled.

## Example Commands

```text
GET,THERMAL,NONE,NONE
GET,THERMAL,TEMPERATURE_C,NONE
SET,THERMAL,ENABLE,TRUE
SET,THERMAL,TELEMETRY,ENABLE
```
