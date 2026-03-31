# MySat Telemetry Specification

## Purpose

This document defines generic outbound telemetry behavior.

Target-specific telemetry fields are documented in:

- [GROUND](./targets/GROUND.md)
- [MODE](./targets/MODE.md)
- [TELEMETRY target](./targets/TELEMETRY_TARGET.md)
- [BATTERY](./targets/BATTERY.md)
- [GPS](./targets/GPS.md)
- [RTC](./targets/RTC.md)
- [THERMAL](./targets/THERMAL.md)
- [IMU](./targets/IMU.md)
- [ADCS](./targets/ADCS.md)
- [STATUS](./targets/STATUS.md)

## Telemetry Line Format

All telemetry lines use:

```text
TIME,TLM,TARGET,PARAMETER,VALUE
```

Field meanings:

| Position | Field | Meaning |
|---|---|---|
| 1 | `TIME` | Device UTC timestamp |
| 2 | `TLM` | Telemetry message type |
| 3 | `TARGET` | Reported subsystem |
| 4 | `PARAMETER` | Reported field |
| 5 | `VALUE` | Current field value |

## Timestamp Rules

- telemetry timestamps come from the device RTC
- timestamps may be wrong until the RTC is synchronised
- multiple lines in the same snapshot can share the same timestamp
- for a given timestamp, the firmware should not emit conflicting values for the same `TARGET` and `PARAMETER`

## Periodic Snapshot Rules

The telemetry scheduler emits periodic snapshots on the configured telemetry interval.

When global telemetry is enabled:

- `STATUS,HEARTBEAT_N` is emitted every interval
- enabled targets are appended to the same snapshot
- a target omitted from periodic telemetry can still be queried with `GET`
- on the RF transport, routine periodic telemetry is skipped when the link is busy rather than queued for later replay

When global telemetry is disabled with `SET,TELEMETRY,ENABLE,FALSE`:

- normal target snapshots stop
- `STATUS,HEARTBEAT_N` still continues at the configured interval

## One-Time Telemetry Events

The firmware also emits one-time telemetry outside the periodic snapshot:

- `STATUS,STARTED,TRUE` during startup
- `RTC,SYNC,TRUE` when the RTC transitions from unsynchronised to synchronised

These events are target-specific and described in their target pages.

## Target Inclusion Rules

- `STATUS` heartbeat is not per-target disableable
- `GROUND` telemetry is host-local to the ground station and is not part of the satellite periodic snapshot scheduler
- `GROUND,TELEMETRY` controls only the ground station's own periodic host-visible status snapshot
- `GROUND,HEARTBEAT_N` is always emitted by the ground station on its heartbeat interval and ignores `GROUND,TELEMETRY`
- `MODE,STATE` is included in normal periodic satellite snapshots while global telemetry is enabled
- `TELEMETRY,SKIPPED_N` reports how many periodic cycles have been dropped by RF busy deferral
- `TELEMETRY`, `BATTERY`, `GPS`, `RTC`, `THERMAL`, `IMU`, and `ADCS` can each be included or omitted from periodic snapshots independently
- `GET,NONE,NONE,NONE` emits a one-time full snapshot across all implemented targets, including `STATUS,HEARTBEAT_N`
- `GET,<target>,NONE,NONE` returns the full implemented telemetry set for that target
- `GET,<target>,<parameter>,NONE` returns a single telemetry line for that field when supported

## Value Conventions

The current protocol uses a naming convention where the parameter name usually carries the unit or data type:

| Suffix | Meaning |
|---|---|
| `_N` | count |
| `_S` | seconds |
| `_V` | volts |
| `_A` | amps |
| `_P` | percent |
| `_D` | decimal degrees |
| `_DEG` | degrees |
| `_M` | metres |
| `_MS2` | metres per second squared |
| `_DPS` | degrees per second |
| `_UT` | microtesla |
| `_KPH` | kilometres per hour |

## Examples

```text
2026-03-27T12:00:00Z,TLM,STATUS,HEARTBEAT_N,12
2026-03-27T12:00:00Z,TLM,RTC,CURRENT_TIME,2026-03-27T12:00:00Z
2026-03-27T12:00:00Z,TLM,RTC,SOURCE,GPS
2026-03-27T12:00:00Z,TLM,BATTERY,ENABLE,TRUE
2026-03-27T12:00:00Z,TLM,BATTERY,STATE,FAST_CHARGE
2026-03-27T12:00:00Z,TLM,BATTERY,HEALTH,OK
2026-03-27T12:00:00Z,TLM,GPS,LATITUDE_D,48.85837
2026-03-27T12:00:00Z,TLM,IMU,Z_MS2,9.807
2026-03-27T12:00:00Z,TLM,IMU,HEADING_DEG,183.4
2026-03-27T12:00:00Z,TLM,ADCS,SOURCE,ACCEL_GYRO_MAG
2026-03-27T12:00:00Z,TLM,ADCS,HEADING_DEG,181.7
2026-03-27T12:00:00Z,TLM,ADCS,ROLL_DEG,-4.52
```
