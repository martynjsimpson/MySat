# IMU Target

## Overview

`IMU` reports acceleration and angular rate from an MPU-6050 plus magnetic field and heading from a QMC5883L on the shared I2C bus, and allows the IMU service to be enabled or disabled.

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
| `GET,IMU,MAG_X_UT,NONE` | Returns X-axis magnetic field in microtesla |
| `GET,IMU,MAG_Y_UT,NONE` | Returns Y-axis magnetic field in microtesla |
| `GET,IMU,MAG_Z_UT,NONE` | Returns Z-axis magnetic field in microtesla |
| `GET,IMU,HEADING_DEG,NONE` | Returns heading estimate in degrees |

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
| `IMU,AVAILABLE` | Whether the last read produced valid MPU-6050 motion data | `TRUE`, `FALSE` |
| `IMU,X_MS2` | Acceleration on the X axis in m/s² | float |
| `IMU,Y_MS2` | Acceleration on the Y axis in m/s² | float |
| `IMU,Z_MS2` | Acceleration on the Z axis in m/s² | float |
| `IMU,GYRO_X_DPS` | Angular rate on the X axis in degrees per second | float |
| `IMU,GYRO_Y_DPS` | Angular rate on the Y axis in degrees per second | float |
| `IMU,GYRO_Z_DPS` | Angular rate on the Z axis in degrees per second | float |
| `IMU,MAG_X_UT` | Magnetic field on the X axis in microtesla | float |
| `IMU,MAG_Y_UT` | Magnetic field on the Y axis in microtesla | float |
| `IMU,MAG_Z_UT` | Magnetic field on the Z axis in microtesla | float |
| `IMU,HEADING_DEG` | Heading estimate in degrees | float |

## Behavior Notes

- The current firmware wakes the MPU-6050 from sleep during startup and reads both accelerometer and gyroscope data over the shared I2C bus at address `0x68`.
- The current firmware also configures a QMC5883L magnetometer on the same I2C bus at address `0x0D` and reports magnetic field plus a simple heading estimate.
- IMU sampling is enabled by default in firmware.
- IMU periodic telemetry is disabled by default until explicitly enabled.
- If the MPU-6050 does not respond on the expected I2C address, `AVAILABLE` remains `FALSE` and IMU values are reported as `0`.
- If the QMC5883L is not present or a magnetometer read fails, `MAG_X_UT`, `MAG_Y_UT`, `MAG_Z_UT`, and `HEADING_DEG` are reported as `0` while MPU-6050 values can still remain valid.
- `HEADING_DEG` is a simple magnetometer heading derived from `atan2(Y, X)` with no declination or tilt compensation applied.
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
GET,IMU,HEADING_DEG,NONE
SET,IMU,ENABLE,TRUE
SET,IMU,TELEMETRY,ENABLE
```
