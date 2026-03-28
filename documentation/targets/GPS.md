# GPS Target

## Overview

`GPS` controls and reports the MKR GPS module connected over the I2C cable path.

## Implemented Commands

### GET

| Command | Result |
|---|---|
| `GET,GPS,NONE,NONE` | Returns all implemented GPS telemetry fields |
| `GET,GPS,TELEMETRY,NONE` | Returns GPS periodic telemetry enable state |
| `GET,GPS,ENABLE,NONE` | Returns whether GPS service is enabled |
| `GET,GPS,AVAILABLE,NONE` | Returns whether a valid recent fix is available |
| `GET,GPS,LATITUDE_D,NONE` | Returns latitude |
| `GET,GPS,LONGITUDE_D,NONE` | Returns longitude |
| `GET,GPS,ALTITUDE_M,NONE` | Returns altitude |
| `GET,GPS,SPEED_KPH,NONE` | Returns speed |
| `GET,GPS,SATELLITES_N,NONE` | Returns visible satellite count |

### SET

| Command | Result |
|---|---|
| `SET,GPS,ENABLE,TRUE` | Enables GPS polling |
| `SET,GPS,ENABLE,FALSE` | Disables GPS polling and clears cached values |
| `SET,GPS,TELEMETRY,ENABLE` | Includes GPS in periodic telemetry |
| `SET,GPS,TELEMETRY,DISABLE` | Omits GPS from periodic telemetry |

## Telemetry Fields

| Field | Meaning | Values |
|---|---|---|
| `GPS,TELEMETRY` | Whether GPS is included in periodic telemetry | `TRUE`, `FALSE` |
| `GPS,ENABLE` | Whether GPS service is enabled | `TRUE`, `FALSE` |
| `GPS,AVAILABLE` | Whether a recent valid fix is available | `TRUE`, `FALSE` |
| `GPS,LATITUDE_D` | Latitude in decimal degrees | float |
| `GPS,LONGITUDE_D` | Longitude in decimal degrees | float |
| `GPS,ALTITUDE_M` | Altitude in metres | float |
| `GPS,SPEED_KPH` | Ground speed in km/h | float |
| `GPS,SATELLITES_N` | Visible satellite count | unsigned integer |

## Behavior Notes

- Coordinates are rounded to the configured decimal precision, currently `5` decimal places.
- Speeds below the configured stationary threshold, currently `1.0` kph, are reported as `0.0`.
- If no recent fix is available, positional fields and satellite count are reported as `0`.
- `GPS,AVAILABLE` means a recent valid fix is available, not just that the module is physically present.

## Examples

```text
SET,GPS,ENABLE,TRUE
GET,GPS,LATITUDE_D,NONE
GET,GPS,NONE,NONE
```
