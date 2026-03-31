# GROUND Target

## Overview

`GROUND` is a host-local protocol target exposed by the ground-station firmware at the USB serial boundary.

It does not exist on the satellite. The ground station uses it to report bridge health, counters, local clock state, and host-side control actions such as reboot.

## Implemented Commands

| Command | Meaning |
|---|---|
| `GET,GROUND,NONE,NONE` | Returns the full implemented ground-station telemetry set |
| `GET,GROUND,HEARTBEAT_N,NONE` | Returns the current ground-station heartbeat count |
| `GET,GROUND,CURRENT_TIME,NONE` | Returns the ground-station current UTC time |
| `GET,GROUND,SOURCE,NONE` | Returns the current ground-station clock source |
| `GET,GROUND,RADIO,NONE` | Returns LoRa bridge readiness |
| `GET,GROUND,PENDING,NONE` | Returns whether a satellite command is currently awaiting completion |
| `GET,GROUND,CLOCK_SYNC,NONE` | Returns whether the ground-station clock has been set from the host |
| `GET,GROUND,TELEMETRY,NONE` | Returns whether periodic ground-station telemetry reporting is enabled |
| `GET,GROUND,TX_PACKETS_N,NONE` | Returns transmitted RF packet count |
| `GET,GROUND,RX_PACKETS_N,NONE` | Returns received RF packet count |
| `GET,GROUND,DROP_PACKETS_N,NONE` | Returns dropped RF packet count |
| `GET,GROUND,LAST_RETRY_N,NONE` | Returns the most recent retry attempt count |
| `SET,GROUND,CURRENT_TIME,<iso8601>` | Sets the ground-station UTC clock from the host |
| `SET,GROUND,TELEMETRY,ENABLE` | Enables periodic ground-station telemetry reporting |
| `SET,GROUND,TELEMETRY,DISABLE` | Disables periodic ground-station telemetry reporting |
| `PING,GROUND,NONE,NONE` | Verifies the host-to-ground-station command path |
| `RESET,GROUND,NONE,NONE` | Reboots the ground-station MCU |

## Telemetry Fields

| Field | Meaning | Typical values |
|---|---|---|
| `GROUND,STARTED` | Startup event emitted by the ground station | `TRUE` |
| `GROUND,HEARTBEAT_N` | Ground-station liveness heartbeat counter | integer |
| `GROUND,CURRENT_TIME` | Current UTC time used in host-local protocol messages | ISO 8601 UTC |
| `GROUND,SOURCE` | Local clock source | `LOCAL`, `UNSYNC` |
| `GROUND,CLOCK_SYNC` | Whether the local clock has been set by the host | `TRUE`, `FALSE` |
| `GROUND,TELEMETRY` | Whether periodic ground-station telemetry reporting is enabled | `TRUE`, `FALSE` |
| `GROUND,RADIO` | LoRa bridge readiness | `READY`, `FAILED` |
| `GROUND,PENDING` | Whether a satellite command is awaiting response | `TRUE`, `FALSE` |
| `GROUND,TX_PACKETS_N` | RF packets transmitted by the ground station | integer |
| `GROUND,RX_PACKETS_N` | RF packets received from the satellite | integer |
| `GROUND,DROP_PACKETS_N` | RF packets dropped locally due to decode failure or oversize packet | integer |
| `GROUND,LAST_RETRY_N` | Most recent retry attempt count | integer |
| `GROUND,LAST_ERROR` | Last local RF decode error category | symbolic token |

## Behavior Notes

- Satellite `ACK`, `ERR`, and `TLM` messages still pass through the ground station unchanged.
- `GROUND` messages are generated only by the ground station itself.
- `GROUND,HEARTBEAT_N` is emitted on every host heartbeat interval regardless of `GROUND,TELEMETRY`.
- The ground station emits periodic `GROUND` telemetry snapshots on its host heartbeat interval when `GROUND,TELEMETRY` is enabled.
- Disabling `GROUND,TELEMETRY` suppresses only the periodic ground-station snapshot. One-time local events such as startup, drop/error reporting, retry reporting, and explicit `GET` responses still emit.
- If the host clock has not yet been set, the ground station timestamps start from `2000-01-01T00:00:00Z` plus uptime.
- Retry and timeout behaviour for satellite-bound commands remains owned by the ground station bridge.
