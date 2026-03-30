# BATTERY Target

## Overview

`BATTERY` reports PMIC-backed battery state from the BQ24195 and the onboard ADC measurement path.
It also allows charge enable and disable control through the PMIC.

## Implemented Commands

### GET

| Command | Result |
|---|---|
| `GET,BATTERY,NONE,NONE` | Returns all implemented battery telemetry fields |
| `GET,BATTERY,TELEMETRY,NONE` | Returns battery periodic telemetry enable state |
| `GET,BATTERY,ENABLE,NONE` | Returns battery charge enable state |
| `GET,BATTERY,AVAILABLE,NONE` | Returns whether a battery is detected |
| `GET,BATTERY,CHARGE_CURRENT_A,NONE` | Returns charge current |
| `GET,BATTERY,CHARGE_VOLTAGE_V,NONE` | Returns charge voltage |
| `GET,BATTERY,CHARGE_PERCENT_P,NONE` | Returns charge percentage |
| `GET,BATTERY,VOLTAGE_V,NONE` | Returns measured battery voltage |

### SET

| Command | Result |
|---|---|
| `SET,BATTERY,ENABLE,TRUE` | Enables battery charging through the PMIC |
| `SET,BATTERY,ENABLE,FALSE` | Disables battery charging through the PMIC |
| `SET,BATTERY,TELEMETRY,ENABLE` | Includes battery in periodic telemetry |
| `SET,BATTERY,TELEMETRY,DISABLE` | Omits battery from periodic telemetry |

## Telemetry Fields

| Field | Meaning | Values |
|---|---|---|
| `BATTERY,TELEMETRY` | Whether battery is included in periodic telemetry | `TRUE`, `FALSE` |
| `BATTERY,ENABLE` | Whether battery charging is enabled | `TRUE`, `FALSE` |
| `BATTERY,AVAILABLE` | Whether a battery is connected | `TRUE`, `FALSE` |
| `BATTERY,CHARGE_CURRENT_A` | Charge current in amps | float |
| `BATTERY,CHARGE_VOLTAGE_V` | Charge voltage in volts | float |
| `BATTERY,CHARGE_PERCENT_P` | Estimated charge percentage | unsigned integer |
| `BATTERY,VOLTAGE_V` | Measured battery voltage | float |

## Behavior Notes

- `BATTERY,AVAILABLE` is derived from the current PMIC behavior on this hardware.
- `BATTERY,ENABLE` controls battery charging through the PMIC rather than disconnecting the battery from the system.
- If battery charging is disabled, `BATTERY,AVAILABLE` is reported as `FALSE` and the numeric battery fields are reported as `0`.
- If no battery is connected, the current, voltage, and percentage fields are reported as `0`.
- `GET` works even when battery periodic telemetry is disabled.

## Examples

```text
GET,BATTERY,VOLTAGE_V,NONE
SET,BATTERY,ENABLE,FALSE
SET,BATTERY,TELEMETRY,DISABLE
GET,BATTERY,NONE,NONE
```
