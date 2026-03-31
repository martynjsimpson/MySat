# MODE Target

## Overview

`MODE` reports and controls the current satellite mission mode.

In the current implementation, mission mode primarily drives the default periodic telemetry set used by the satellite. Operators can still override per-target telemetry with `SET,<target>,TELEMETRY,...` after selecting a mode.

## Implemented Commands

### GET

| Command | Result |
|---|---|
| `GET,MODE,NONE,NONE` | Returns the current mission mode |
| `GET,MODE,STATE,NONE` | Returns the current mission mode |
| `GET,MODE,TELEMETRY,NONE` | Returns whether `MODE` is included in periodic telemetry |

### SET

| Command | Result |
|---|---|
| `SET,MODE,STATE,LAUNCH` | Applies launch telemetry defaults |
| `SET,MODE,STATE,ORBIT` | Applies orbit telemetry defaults |
| `SET,MODE,STATE,LOW_POWER` | Applies low-power telemetry defaults |
| `SET,MODE,TELEMETRY,ENABLE` | Includes `MODE` in periodic telemetry |
| `SET,MODE,TELEMETRY,DISABLE` | Omits `MODE` from periodic telemetry |

## Telemetry Fields

| Field | Meaning | Values |
|---|---|---|
| `MODE,TELEMETRY` | Whether `MODE` is included in periodic telemetry | `TRUE`, `FALSE` |
| `MODE,STATE` | Current mission mode | `LAUNCH`, `ORBIT`, `LOW_POWER` |

## Behavior Notes

- The current firmware defaults to `ORBIT` mode at startup.
- Changing mode reapplies the telemetry defaults for that mode.
- Manual per-target telemetry overrides still work after a mode change.
- `MODE` behaves like the other targets for `SET,MODE,TELEMETRY,...` control.
- When `MODE` telemetry is enabled and global telemetry is enabled, the firmware emits both `MODE,TELEMETRY` and `MODE,STATE` in routine periodic telemetry.

### Current Mode Defaults

| Mode | Default periodic targets |
|---|---|
| `LAUNCH` | `MODE`, `STATUS`, `RTC`, `BATTERY`, `IMU`, `ADCS` |
| `ORBIT` | `MODE`, `STATUS`, `RTC`, `BATTERY`, `GPS`, `ADCS` |
| `LOW_POWER` | `MODE`, `STATUS`, `RTC`, `BATTERY` |

`STATUS` heartbeat remains outside per-target telemetry control and continues even when normal global telemetry is disabled.

## Examples

```text
GET,MODE,NONE,NONE
GET,MODE,STATE,NONE
SET,MODE,STATE,LAUNCH
SET,MODE,STATE,LOW_POWER
```
