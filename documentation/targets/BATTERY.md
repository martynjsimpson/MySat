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
| `GET,BATTERY,STATE,NONE` | Returns charger state summary |
| `GET,BATTERY,HEALTH,NONE` | Returns battery health summary |
| `GET,BATTERY,CHARGE_CURRENT_A,NONE` | Returns charge current |
| `GET,BATTERY,CHARGE_VOLTAGE_V,NONE` | Returns charge voltage |

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
| `BATTERY,STATE` | Current PMIC charging state summary | `DISABLED`, `ABSENT`, `NOT_CHARGING`, `PRE_CHARGE`, `FAST_CHARGE`, `CHARGE_DONE`, `FAULT` |
| `BATTERY,HEALTH` | Current PMIC-backed health summary | `NONE`, `OK`, `LOW_POWER`, `FAIL` |
| `BATTERY,CHARGE_CURRENT_A` | Charge current in amps | float |
| `BATTERY,CHARGE_VOLTAGE_V` | Charge voltage in volts | float |

## Behavior Notes

- `BATTERY,AVAILABLE` is derived from the current PMIC behavior on this hardware.
- `BATTERY,ENABLE` controls battery charging through the PMIC rather than disconnecting the battery from the system.
- If battery charging is disabled, `BATTERY,AVAILABLE` is reported as `FALSE` and PMIC-backed numeric battery fields are reported as `0`.
- If no battery is connected, PMIC-backed numeric battery fields are reported as `0`.
- `BATTERY,STATE` is derived from the PMIC charging-status register and reports coarse charger lifecycle state.
- `BATTERY,HEALTH` is derived from PMIC status and fault bits and currently reports `LOW_POWER` when the PMIC indicates the battery is below the configured minimum system voltage, and `FAIL` when battery over-voltage or charge-fault conditions are reported.
- `GET` works even when battery periodic telemetry is disabled.

## Examples

```text
GET,BATTERY,STATE,NONE
GET,BATTERY,HEALTH,NONE
SET,BATTERY,ENABLE,FALSE
SET,BATTERY,TELEMETRY,DISABLE
GET,BATTERY,NONE,NONE
```
