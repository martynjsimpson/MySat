# STATUS Target

## Overview

`STATUS` reports startup and heartbeat state for the firmware.

## Implemented Commands

### GET

| Command | Result |
|---|---|
| `GET,STATUS,NONE,NONE` | Returns the current heartbeat value |
| `GET,STATUS,HEARTBEAT_N,NONE` | Returns the current heartbeat value |

### SET

No `SET` commands are currently implemented for `STATUS`.

## Telemetry Fields

| Field | Meaning | Values |
|---|---|---|
| `STATUS,STARTED` | One-time startup event | `TRUE` |
| `STATUS,HEARTBEAT_N` | Heartbeat counter | unsigned integer |

## Behavior Notes

- `STATUS,STARTED,TRUE` is emitted once during startup.
- The current firmware emits `STATUS,STARTED,TRUE` and then `STATUS,HEARTBEAT_N,0` during startup initialisation.
- `STATUS,HEARTBEAT_N` increments on each periodic heartbeat emission.
- The heartbeat continues on the configured interval even when normal global telemetry is disabled.
- `GET` returns the current heartbeat value without incrementing it.

## Examples

```text
GET,STATUS,HEARTBEAT_N,NONE
SET,TELEMETRY,ENABLE,FALSE
RESET,NONE,NONE,NONE
```
