# MySat Communication Protocol

## Purpose

This document defines the generic wire protocol used between the firmware and the ground station.

Target-specific command and telemetry details are documented separately:

- [TELEMETRY target](./targets/TELEMETRY_TARGET.md)
- [BATTERY](./targets/BATTERY.md)
- [GPS](./targets/GPS.md)
- [RTC](./targets/RTC.md)
- [THERMAL](./targets/THERMAL.md)
- [IMU](./targets/IMU.md)
- [ADCS](./targets/ADCS.md)
- [STATUS](./targets/STATUS.md)

Telemetry framing and snapshot behavior are documented in [Telemetry.md](./Telemetry.md).

## Command Format

All inbound commands use the same four-field form:

```text
COMMAND,TARGET,PARAMETER,VALUE
```

Rules:

- fields are comma-separated
- commands are newline-terminated
- `VALUE` may be symbolic or numeric depending on the command
- `NONE` is used when a field is intentionally unused

## Response Format

All outbound messages are timestamped:

```text
TIME,TYPE,...
```

`TIME` is the device RTC time in ISO 8601 UTC form:

```text
yyyy-mm-ddThh:mm:ssZ
```

`TYPE` is one of:

- `ACK`
- `ERR`
- `TLM`

## Response Types

`ACK` confirms a command was accepted:

```text
TIME,ACK,TARGET,VALUE
```

`ERR` reports a rejected or malformed command:

```text
TIME,ERR,ERROR_CODE[,ENCODED_COMMAND]
```

`TLM` reports telemetry:

```text
TIME,TLM,TARGET,PARAMETER,VALUE
```

## Generic Command Rules

- `GET,<target>,NONE,NONE` returns the full implemented telemetry set for that target.
- `GET,NONE,NONE,NONE` returns a one-time full snapshot across all implemented targets.
- `GET,<target>,<parameter>,NONE` returns one telemetry line for that parameter when supported by that target.
- Per-target telemetry disable only removes that target from periodic snapshots.
- `SET,<target>,TELEMETRY,<value>` currently accepts either `ENABLE` and `DISABLE` or `TRUE` and `FALSE`.
- `GET` still works even when that target is omitted from periodic telemetry.
- `SET,TELEMETRY,ENABLE,FALSE` disables normal periodic telemetry, but the `STATUS` heartbeat still continues on the configured interval.

## Implemented Command Types

| Token | Meaning |
|---|---|
| `SET` | Apply a value to a target |
| `GET` | Request target state |
| `PING` | Verify the command path |
| `RESET` | Reboot the device |

## Reserved Command Types

| Token | Meaning |
|---|---|
| `SAVE` | Reserved for future persistence features |

## Implemented Targets

| Target | Purpose | Reference |
|---|---|---|
| `TELEMETRY` | Global and per-target telemetry control | [TELEMETRY_TARGET.md](./targets/TELEMETRY_TARGET.md) |
| `BATTERY` | PMIC-backed battery reporting and charge control | [BATTERY.md](./targets/BATTERY.md) |
| `GPS` | GPS control and position reporting | [GPS.md](./targets/GPS.md) |
| `RTC` | RTC time and sync control | [RTC.md](./targets/RTC.md) |
| `THERMAL` | DHT11-based temperature and humidity reporting | [THERMAL.md](./targets/THERMAL.md) |
| `IMU` | MPU-6050 motion and QMC5883L magnetic field reporting | [IMU.md](./targets/IMU.md) |
| `ADCS` | Derived attitude, heading, and yaw-rate reporting | [ADCS.md](./targets/ADCS.md) |
| `STATUS` | Startup event and heartbeat reporting | [STATUS.md](./targets/STATUS.md) |

## Reserved Targets

| Token | Intended future meaning |
|---|---|
| `MODE` | Spacecraft operating mode |
| `RADIO` | Communications subsystem |
| `POWER` | Power subsystem |
| `PAYLOAD` | Payload subsystem |
| `LOG` | Log or event store |
| `WATCHDOG` | Fault monitoring or watchdog |
| `UPTIME` | Reserved for future uptime reporting |

## Implemented Generic Tokens

These tokens are implemented in the parser today and are used by one or more targets.

| Kind | Tokens |
|---|---|
| Parameters | `NONE`, `STATE`, `ENABLE`, `INTERVAL_S`, `TELEMETRY`, `CURRENT_TIME`, `HEARTBEAT_N`, `SYNC`, `SOURCE`, `AVAILABLE`, `HEALTH`, `CHARGE_CURRENT_A`, `CHARGE_VOLTAGE_V`, `LATITUDE_D`, `LONGITUDE_D`, `ALTITUDE_M`, `SPEED_KPH`, `SATELLITES_N`, `TEMPERATURE_C`, `HUMIDITY_P`, `X_MS2`, `Y_MS2`, `Z_MS2`, `GYRO_X_DPS`, `GYRO_Y_DPS`, `GYRO_Z_DPS`, `MAG_X_UT`, `MAG_Y_UT`, `MAG_Z_UT`, `HEADING_DEG`, `ROLL_DEG`, `PITCH_DEG`, `YAW_RATE_DPS` |
| Values | `NONE`, `TRUE`, `FALSE`, `ENABLE`, `DISABLE`, `ON`, `OFF`, `OK`, `FAIL`, `LOW_POWER` |

## Reserved Generic Tokens

These parser tokens are reserved but not used by the current firmware features.

| Kind | Tokens |
|---|---|
| Parameters | `MODE`, `UPTIME_S` |
| Values | `SAFE`, `NORMAL`, `ACTIVE`, `IDLE` |

## Error Handling

Common error codes currently emitted by the firmware include:

- `BAD_FORMAT`
- `BAD_PARAMETER`
- `BAD_VALUE`
- `UNKNOWN_CMD`
- `UNKNOWN_TARGET`

Additional target-specific errors are documented on the relevant target pages.

## Examples

```text
GET,NONE,NONE,NONE
GET,GPS,LATITUDE_D,NONE
SET,RTC,SYNC,GPS
SET,TELEMETRY,ENABLE,FALSE
PING,NONE,NONE,NONE
RESET,NONE,NONE,NONE
```
