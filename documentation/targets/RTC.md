# RTC Target

## Overview

`RTC` reports device time and synchronisation state, and accepts both manual time setting and GPS-based sync.

## Implemented Commands

### GET

| Command | Result |
|---|---|
| `GET,RTC,NONE,NONE` | Returns all implemented RTC telemetry fields |
| `GET,RTC,TELEMETRY,NONE` | Returns RTC periodic telemetry enable state |
| `GET,RTC,CURRENT_TIME,NONE` | Returns current RTC time |
| `GET,RTC,SYNC,NONE` | Returns current sync state |
| `GET,RTC,SOURCE,NONE` | Returns the current clock source |

### SET

| Command | Result |
|---|---|
| `SET,RTC,CURRENT_TIME,<iso8601>` | Sets the RTC from an ISO UTC timestamp |
| `SET,RTC,SYNC,GPS` | Sets the RTC from current GPS time if valid |
| `SET,RTC,TELEMETRY,ENABLE` | Includes RTC in periodic telemetry |
| `SET,RTC,TELEMETRY,DISABLE` | Omits RTC from periodic telemetry |

## Telemetry Fields

| Field | Meaning | Values |
|---|---|---|
| `RTC,TELEMETRY` | Whether RTC is included in periodic telemetry | `TRUE`, `FALSE` |
| `RTC,CURRENT_TIME` | Current RTC time | ISO 8601 UTC timestamp |
| `RTC,SYNC` | Whether the RTC is considered synchronised | `TRUE`, `FALSE` |
| `RTC,SOURCE` | Current source of the RTC value | `NONE`, `LOCAL`, `GPS` |

## Behavior Notes

- `RTC,SYNC,TRUE` is emitted immediately when the clock transitions from unsynchronised to synchronised.
- `RTC,SOURCE` reports where the current clock value most recently came from.
- `SET,RTC,CURRENT_TIME,<iso8601>` sets `RTC,SOURCE` to `LOCAL`.
- `SET,RTC,SYNC,GPS` and automatic GPS resync set `RTC,SOURCE` to `GPS`.
- GPS auto-sync is present in firmware as a config-controlled behavior.
- While unsynchronised, the firmware will set the RTC from GPS as soon as valid GPS time is available.
- Once synchronised, the firmware checks for RTC drift against GPS on a fixed interval instead of every loop iteration.
- A drift correction is only applied if the RTC differs from GPS by more than one second, the condition is observed on two consecutive scheduled checks, and the minimum resync cooldown has elapsed.
- When an automatic drift correction is applied, the firmware emits `RTC,CURRENT_TIME` telemetry immediately so the ground side can observe the adjustment.
- `SET,RTC,SYNC,GPS` returns `ERR,GPS_TIME_UNAVAILABLE` if valid GPS time is not available.
- `GET` works even when RTC periodic telemetry is disabled.

## Examples

```text
SET,RTC,CURRENT_TIME,2026-03-27T12:00:00Z
SET,RTC,SYNC,GPS
GET,RTC,SOURCE,NONE
GET,RTC,NONE,NONE
```
