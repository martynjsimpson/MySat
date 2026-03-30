# IMU Target

## Overview

`IMU` reports acceleration and angular rate from an MPU-6050 connected to the shared I2C bus and allows the IMU service to be enabled or disabled.

## Implemented Commands

### GET

| Command | Result |
|---|---|
| `GET,IMU,NONE,NONE` | Returns all implemented IMU telemetry fields |
| `GET,IMU,TELEMETRY,NONE` | Returns IMU periodic telemetry enable state |
| `GET,IMU,ENABLE,NONE` | Returns whether IMU sampling is enabled |
| `GET,IMU,AVAILABLE,NONE` | Returns whether a valid recent IMU read is available |
| `GET,IMU,X_MS2,NONE` | Returns X-axis acceleration in metres per second squared |
| `GET,IMU,Y_MS2,NONE` | Returns Y-axis acceleration in metres per second squared |
| `GET,IMU,Z_MS2,NONE` | Returns Z-axis acceleration in metres per second squared |
| `GET,IMU,GYRO_X_DPS,NONE` | Returns X-axis angular rate in degrees per second |
| `GET,IMU,GYRO_Y_DPS,NONE` | Returns Y-axis angular rate in degrees per second |
| `GET,IMU,GYRO_Z_DPS,NONE` | Returns Z-axis angular rate in degrees per second |

### SET

| Command | Result |
|---|---|
| `SET,IMU,ENABLE,TRUE` | Enables IMU sampling |
| `SET,IMU,ENABLE,FALSE` | Disables IMU sampling and clears availability |
| `SET,IMU,TELEMETRY,ENABLE` | Includes IMU data in periodic telemetry |
| `SET,IMU,TELEMETRY,DISABLE` | Omits IMU data from periodic telemetry |

## Telemetry Fields

| Field | Meaning | Values |
|---|---|---|
| `IMU,TELEMETRY` | Whether IMU is included in periodic telemetry | `TRUE`, `FALSE` |
| `IMU,ENABLE` | Whether IMU sampling is enabled | `TRUE`, `FALSE` |
| `IMU,AVAILABLE` | Whether the last read produced valid acceleration data | `TRUE`, `FALSE` |
| `IMU,X_MS2` | Acceleration on the X axis in m/s² | float |
| `IMU,Y_MS2` | Acceleration on the Y axis in m/s² | float |
| `IMU,Z_MS2` | Acceleration on the Z axis in m/s² | float |
| `IMU,GYRO_X_DPS` | Angular rate on the X axis in degrees per second | float |
| `IMU,GYRO_Y_DPS` | Angular rate on the Y axis in degrees per second | float |
| `IMU,GYRO_Z_DPS` | Angular rate on the Z axis in degrees per second | float |

## Behavior Notes

- The current firmware wakes the MPU-6050 from sleep during startup and reads both accelerometer and gyroscope data over the shared I2C bus at address `0x68`.
- IMU sampling is enabled by default in firmware.
- IMU periodic telemetry is disabled by default until explicitly enabled.
- If the sensor does not respond on the expected I2C address, `AVAILABLE` remains `FALSE` and axis values are reported as `0`.
- When IMU sampling is disabled with `SET,IMU,ENABLE,FALSE`, availability and cached axis values are cleared immediately.
- `GET` works even when IMU periodic telemetry is disabled.

## Wiring

- `VCC` -> MKR `3.3V`
- `GND` -> MKR `GND`
- `SDA` -> MKR `SDA`
- `SCL` -> MKR `SCL`

## Examples

```text
GET,IMU,NONE,NONE
GET,IMU,Z_MS2,NONE
GET,IMU,GYRO_Z_DPS,NONE
SET,IMU,ENABLE,TRUE
SET,IMU,TELEMETRY,ENABLE
```
