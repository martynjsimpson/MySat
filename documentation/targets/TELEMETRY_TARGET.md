# TELEMETRY Target

## Overview

`TELEMETRY` controls the periodic telemetry scheduler and reports telemetry configuration state.

## Implemented Commands

### GET

| Command | Result |
|---|---|
| `GET,TELEMETRY,NONE,NONE` | Returns all implemented telemetry-control fields |
| `GET,TELEMETRY,TELEMETRY,NONE` | Returns whether telemetry-about-telemetry is included in snapshots |
| `GET,TELEMETRY,ENABLE,NONE` | Returns global periodic telemetry enable state |
| `GET,TELEMETRY,INTERVAL_S,NONE` | Returns telemetry interval in seconds |
| `GET,TELEMETRY,SKIPPED_N,NONE` | Returns how many periodic snapshots have been skipped because the RF link was busy |
| `GET,TELEMETRY,LAST_SKIP_REASON,NONE` | Returns the last recorded reason a periodic telemetry cycle was skipped |

### SET

| Command | Result |
|---|---|
| `SET,TELEMETRY,ENABLE,TRUE` | Enables normal periodic telemetry |
| `SET,TELEMETRY,ENABLE,FALSE` | Disables normal periodic telemetry |
| `SET,TELEMETRY,INTERVAL_S,<n>` | Sets periodic interval, currently `1` to `3600` seconds |
| `SET,TELEMETRY,TELEMETRY,ENABLE` | Includes the `TELEMETRY` target in periodic snapshots |
| `SET,TELEMETRY,TELEMETRY,DISABLE` | Omits the `TELEMETRY` target from periodic snapshots |

Per-target telemetry control for other targets uses the same `SET,<target>,TELEMETRY,<value>` pattern and is documented on each target page.

## Telemetry Fields

| Field | Meaning | Values |
|---|---|---|
| `TELEMETRY,TELEMETRY` | Whether the `TELEMETRY` target reports in periodic snapshots | `TRUE`, `FALSE` |
| `TELEMETRY,ENABLE` | Whether normal periodic telemetry is enabled | `TRUE`, `FALSE` |
| `TELEMETRY,INTERVAL_S` | Snapshot interval in seconds | unsigned integer |
| `TELEMETRY,SKIPPED_N` | Count of periodic telemetry cycles skipped due to transport busy deferral | unsigned integer |
| `TELEMETRY,LAST_SKIP_REASON` | Last recorded reason a periodic telemetry cycle was skipped | `NONE`, `BUSY` |

## Behavior Notes

- `SET,TELEMETRY,ENABLE,FALSE` stops normal periodic snapshots.
- `STATUS,HEARTBEAT_N` still continues on the same interval when global telemetry is disabled.
- `TELEMETRY,SKIPPED_N` increments when a periodic telemetry cycle is deferred by the RF `skip-if-busy` rule.
- `TELEMETRY,LAST_SKIP_REASON` currently records `BUSY` when the transport defer rule skips a cycle.
- `MODE` can reapply a baseline per-target telemetry set, after which manual `SET,<target>,TELEMETRY,...` overrides still work.
- `GET` works regardless of periodic telemetry state.

## Examples

```text
SET,TELEMETRY,INTERVAL_S,5
SET,TELEMETRY,ENABLE,FALSE
GET,TELEMETRY,NONE,NONE
```
