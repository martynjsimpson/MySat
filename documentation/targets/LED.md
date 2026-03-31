# LED Target

## Overview

`LED` controls and reports the built-in LED state used for development and protocol testing.

## Implemented Commands

### GET

| Command | Result |
|---|---|
| `GET,LED,NONE,NONE` | Returns all implemented LED telemetry fields |
| `GET,LED,TELEMETRY,NONE` | Returns periodic LED telemetry enable state |
| `GET,LED,ENABLE,NONE` | Returns whether LED operation is enabled |
| `GET,LED,STATE,NONE` | Returns current LED on/off state |
| `GET,LED,COLOR,NONE` | Returns current color token |

### SET

| Command | Result |
|---|---|
| `SET,LED,ENABLE,TRUE` | Enables LED control |
| `SET,LED,ENABLE,FALSE` | Disables LED control and forces LED off |
| `SET,LED,STATE,ON` | Turns LED output on |
| `SET,LED,STATE,OFF` | Turns LED output off |
| `SET,LED,COLOR,RED` | Sets color to red |
| `SET,LED,COLOR,GREEN` | Sets color to green |
| `SET,LED,COLOR,BLUE` | Sets color to blue |
| `SET,LED,TELEMETRY,ENABLE` | Includes LED in periodic telemetry |
| `SET,LED,TELEMETRY,DISABLE` | Omits LED from periodic telemetry |

## Telemetry Fields

| Field | Meaning | Values |
|---|---|---|
| `LED,TELEMETRY` | Whether LED is included in periodic telemetry | `TRUE`, `FALSE` |
| `LED,ENABLE` | Whether LED control is enabled | `TRUE`, `FALSE` |
| `LED,STATE` | Current output state | `ON`, `OFF` |
| `LED,COLOR` | Current logical color selection | `RED`, `GREEN`, `BLUE` |

## Behavior Notes

- `SET,LED,ENABLE,FALSE` also forces `LED,STATE` to `OFF`.
- `GET` works even when LED periodic telemetry is disabled.
- `SET,LED,STATE,ON` returns `ERR,LED_DISABLED` if LED control is disabled.
- On the MKR WAN 1310, hardware LED output is built-in LED only. `COLOR` is retained as a protocol-compatible state token and no longer maps to distinct onboard RGB hardware.

## Examples

```text
SET,LED,ENABLE,TRUE
SET,LED,COLOR,BLUE
GET,LED,NONE,NONE
```
