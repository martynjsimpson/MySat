# ADCS Target

## Overview

`ADCS` stands for Attitude Determination and Control System. It reports interpreted attitude-related values derived from the `IMU` target and allows the ADCS service to be enabled or disabled.

## Implemented Commands

### GET

| Command | Result |
|---|---|
| `GET,ADCS,NONE,NONE` | Returns all implemented ADCS telemetry fields |
| `GET,ADCS,TELEMETRY,NONE` | Returns ADCS periodic telemetry enable state |
| `GET,ADCS,ENABLE,NONE` | Returns whether ADCS processing is enabled |
| `GET,ADCS,AVAILABLE,NONE` | Returns whether valid recent ADCS values are available |
| `GET,ADCS,ROLL_DEG,NONE` | Returns roll angle in degrees |
| `GET,ADCS,PITCH_DEG,NONE` | Returns pitch angle in degrees |
| `GET,ADCS,YAW_RATE_DPS,NONE` | Returns yaw rate in degrees per second |

### SET

| Command | Result |
|---|---|
| `SET,ADCS,ENABLE,TRUE` | Enables ADCS processing |
| `SET,ADCS,ENABLE,FALSE` | Disables ADCS processing and clears availability |
| `SET,ADCS,TELEMETRY,ENABLE` | Includes ADCS data in periodic telemetry |
| `SET,ADCS,TELEMETRY,DISABLE` | Omits ADCS data from periodic telemetry |

## Telemetry Fields

| Field | Meaning | Values |
|---|---|---|
| `ADCS,TELEMETRY` | Whether ADCS is included in periodic telemetry | `TRUE`, `FALSE` |
| `ADCS,ENABLE` | Whether ADCS processing is enabled | `TRUE`, `FALSE` |
| `ADCS,AVAILABLE` | Whether the last derived ADCS values are valid | `TRUE`, `FALSE` |
| `ADCS,ROLL_DEG` | Roll angle estimate in degrees | float |
| `ADCS,PITCH_DEG` | Pitch angle estimate in degrees | float |
| `ADCS,YAW_RATE_DPS` | Yaw rate estimate in degrees per second | float |

## Behavior Notes

- The current ADCS implementation depends on the `IMU` target for its source data.
- `ROLL_DEG` and `PITCH_DEG` are currently derived from accelerometer data.
- `YAW_RATE_DPS` is currently derived from the IMU Z-axis gyroscope rate, not from an absolute yaw estimate.
- If the IMU is disabled or unavailable, `ADCS,AVAILABLE` becomes `FALSE` and derived numeric values are reported as `0`.
- ADCS processing is enabled by default in firmware.
- ADCS periodic telemetry is disabled by default until explicitly enabled.
- `GET` works even when ADCS periodic telemetry is disabled.

## Examples

```text
GET,ADCS,NONE,NONE
GET,ADCS,ROLL_DEG,NONE
GET,ADCS,YAW_RATE_DPS,NONE
SET,ADCS,ENABLE,TRUE
SET,ADCS,TELEMETRY,ENABLE
```
