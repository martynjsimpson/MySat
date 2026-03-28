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

## Behavior Notes

- `RTC,SYNC,TRUE` is emitted immediately when the clock transitions from unsynchronised to synchronised.
- GPS auto-sync is also present in firmware as a config-controlled behavior.
- `SET,RTC,SYNC,GPS` returns `ERR,GPS_TIME_UNAVAILABLE` if valid GPS time is not available.
- `GET` works even when RTC periodic telemetry is disabled.

## Examples

```text
SET,RTC,CURRENT_TIME,2026-03-27T12:00:00Z
SET,RTC,SYNC,GPS
GET,RTC,NONE,NONE
```
