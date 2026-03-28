# MySat Telemetry Specification

## Purpose

This document describes the outbound telemetry format used by MySat and provides practical guidance for decoding it in a future ground station or tools such as Serial Studio.

This document builds on the shared message model defined in [Protocol.md](./Protocol.md) and focuses only on telemetry behavior.

---

## Telemetry Line Format

All telemetry lines follow this structure:

```text
TIME,TLM,TARGET,PARAMETER,VALUE
```

### Field meanings

| Position | Field | Meaning |
|---|---|---|
| 1 | `TIME` | UTC timestamp formatted as `yyyy-mm-ddThh:mm:ssZ` |
| 2 | `TLM` | Message type identifier for telemetry |
| 3 | `TARGET` | The subsystem or domain being reported |
| 4 | `PARAMETER` | The property being reported |
| 5 | `VALUE` | The current value of that property |

### Example lines

```text
2026-03-27T12:02:20Z,TLM,RTC,TELEMETRY,TRUE
2026-03-27T12:02:20Z,TLM,RTC,CURRENT_TIME,2026-03-27T12:02:20Z
2026-03-27T12:02:20Z,TLM,RTC,SYNC,TRUE
2026-03-27T12:02:20Z,TLM,LED,ENABLE,FALSE
2026-03-27T12:02:20Z,TLM,LED,STATE,OFF
2026-03-27T12:02:20Z,TLM,LED,COLOR,GREEN
2026-03-27T12:02:20Z,TLM,STATUS,HEARTBEAT_N,12
2026-03-27T12:02:20Z,TLM,TELEMETRY,TELEMETRY,TRUE
2026-03-27T12:02:20Z,TLM,TELEMETRY,ENABLE,TRUE
2026-03-27T12:02:20Z,TLM,TELEMETRY,INTERVAL_S,5
2026-03-27T12:02:20Z,TLM,BATTERY,TELEMETRY,TRUE
2026-03-27T12:02:20Z,TLM,BATTERY,AVAILABLE,TRUE
2026-03-27T12:02:20Z,TLM,BATTERY,CHARGE_CURRENT_A,0.500
2026-03-27T12:02:20Z,TLM,BATTERY,CHARGE_VOLTAGE_V,4.200
2026-03-27T12:02:20Z,TLM,BATTERY,CHARGE_PERCENT_P,87
2026-03-27T12:02:20Z,TLM,BATTERY,VOLTAGE_V,4.010
```

---

## Timestamp Rules

`TIME` is:

- device UTC time formatted as `yyyy-mm-ddThh:mm:ssZ`
- shared by all messages emitted in the same second
- derived from the device RTC
- potentially wrong until the RTC has been synchronised

### Important interpretation rule

Within a single timestamp, telemetry must not contain multiple conflicting values for the same `TARGET + PARAMETER` pair.

This is acceptable:

```text
2026-03-27T12:02:20Z,TLM,LED,ENABLE,FALSE
2026-03-27T12:02:20Z,TLM,LED,STATE,OFF
```

This is not acceptable:

```text
2026-03-27T12:02:20Z,TLM,LED,STATE,OFF
2026-03-27T12:02:20Z,TLM,LED,STATE,ON
```

because the ground station would not know which one is newer.

---

## Telemetry Snapshot Concept

Periodic telemetry is emitted as a **snapshot**.

A telemetry snapshot is the set of telemetry lines emitted together at one timestamp to represent the current device state.

At present, the periodic snapshot includes:

- status heartbeat counter
- RTC time and sync state
- LED status
- telemetry configuration/status, if telemetry-target telemetry is enabled
- battery / PMIC status

If telemetry for an individual target is disabled, that target is omitted from periodic snapshots until it is re-enabled.

For `LED`, `RTC`, and `TELEMETRY`, explicit `GET` commands still return current status even when that target is omitted from periodic snapshots. `BATTERY` currently uses the same gating for `GET` and periodic output, so `GET,BATTERY,NONE,NONE` emits no battery lines while battery telemetry is disabled. `STATUS` is always included in periodic snapshots when global telemetry is enabled, and `GET,STATUS,HEARTBEAT_N,NONE` returns the current counter without incrementing it.

Typical snapshot:

```text
2026-03-27T12:03:20Z,TLM,LED,TELEMETRY,TRUE
2026-03-27T12:03:20Z,TLM,LED,ENABLE,TRUE
2026-03-27T12:03:20Z,TLM,LED,STATE,ON
2026-03-27T12:03:20Z,TLM,LED,COLOR,RED
2026-03-27T12:03:20Z,TLM,STATUS,HEARTBEAT_N,13
2026-03-27T12:03:20Z,TLM,TELEMETRY,TELEMETRY,TRUE
2026-03-27T12:03:20Z,TLM,TELEMETRY,ENABLE,TRUE
2026-03-27T12:03:20Z,TLM,TELEMETRY,INTERVAL_S,5
2026-03-27T12:03:20Z,TLM,BATTERY,TELEMETRY,TRUE
2026-03-27T12:03:20Z,TLM,BATTERY,AVAILABLE,TRUE
2026-03-27T12:03:20Z,TLM,BATTERY,CHARGE_CURRENT_A,0.500
2026-03-27T12:03:20Z,TLM,BATTERY,CHARGE_VOLTAGE_V,4.200
2026-03-27T12:03:20Z,TLM,BATTERY,CHARGE_PERCENT_P,87
2026-03-27T12:03:20Z,TLM,BATTERY,VOLTAGE_V,4.010
```

---

## Current Targets and Parameters

## STATUS target

### `STATUS,HEARTBEAT_N`

Reports the heartbeat counter emitted with each periodic telemetry snapshot.

Value type:
- unsigned integer

Examples:

```text
2026-03-27T12:03:19Z,TLM,STATUS,HEARTBEAT_N,12
2026-03-27T12:03:20Z,TLM,STATUS,HEARTBEAT_N,13
```

The counter increments when a periodic snapshot is emitted. An explicit `GET,STATUS,HEARTBEAT_N,NONE` returns the current value without incrementing it.

## LED target

### `LED,ENABLE`

Reports whether LED operation is currently permitted.

Value type:
- `TRUE`
- `FALSE`

Examples:

```text
2026-03-27T12:03:40Z,TLM,LED,ENABLE,TRUE
2026-03-27T12:03:40Z,TLM,LED,ENABLE,FALSE
```

### `LED,TELEMETRY`

Reports whether LED data is included in periodic telemetry snapshots.

Value type:
- `TRUE`
- `FALSE`

Examples:

```text
2026-03-27T12:03:40Z,TLM,LED,TELEMETRY,TRUE
2026-03-27T12:03:40Z,TLM,LED,TELEMETRY,FALSE
```

### `LED,STATE`

Reports whether the LED output is currently on or off.

Value type:
- `ON`
- `OFF`

Examples:

```text
2026-03-27T12:03:41Z,TLM,LED,STATE,ON
2026-03-27T12:03:41Z,TLM,LED,STATE,OFF
```

### `LED,COLOR`

Reports the currently selected LED color token.

Value type:
- `RED`
- `GREEN`
- `BLUE`

Examples:

```text
2026-03-27T12:03:42Z,TLM,LED,COLOR,RED
2026-03-27T12:03:42Z,TLM,LED,COLOR,GREEN
```

---

## RTC target
### `RTC,TELEMETRY`

Reports whether RTC data is included in periodic telemetry snapshots.

Value type:
- `TRUE`
- `FALSE`

Examples:

```text
2026-03-27T12:03:42Z,TLM,RTC,TELEMETRY,TRUE
2026-03-27T12:03:42Z,TLM,RTC,TELEMETRY,FALSE
```

### `RTC,CURRENT_TIME`

Reports the device's current UTC time.

Value type:
- ISO 8601 UTC timestamp in the form `yyyy-mm-ddThh:mm:ssZ`

Examples:

```text
2026-03-27T12:03:42Z,TLM,RTC,CURRENT_TIME,2026-03-27T12:03:42Z
2026-03-27T12:10:00Z,TLM,RTC,CURRENT_TIME,2026-03-27T12:10:00Z
```

### `RTC,SYNC`

Reports whether the device clock is considered in sync.

Value type:
- `TRUE`
- `FALSE`

Examples:

```text
2026-03-27T12:03:42Z,TLM,RTC,SYNC,TRUE
2000-01-01T00:00:05Z,TLM,RTC,SYNC,FALSE
```

---

## TELEMETRY target

### `TELEMETRY,TELEMETRY`

Reports whether telemetry-about-telemetry is included in periodic snapshots.

Value type:
- `TRUE`
- `FALSE`

Examples:

```text
2026-03-27T12:03:49Z,TLM,TELEMETRY,TELEMETRY,TRUE
2026-03-27T12:03:49Z,TLM,TELEMETRY,TELEMETRY,FALSE
```

### `TELEMETRY,ENABLE`

Reports whether periodic telemetry streaming is enabled.

Value type:
- `TRUE`
- `FALSE`

Examples:

```text
2026-03-27T12:03:50Z,TLM,TELEMETRY,ENABLE,TRUE
2026-03-27T12:03:50Z,TLM,TELEMETRY,ENABLE,FALSE
```

### `TELEMETRY,INTERVAL_S`

Reports the periodic telemetry interval in seconds.

Value type:
- unsigned integer

Examples:

```text
2026-03-27T12:03:51Z,TLM,TELEMETRY,INTERVAL_S,5
2026-03-27T12:03:51Z,TLM,TELEMETRY,INTERVAL_S,10
```

---

## BATTERY target

### `BATTERY,AVAILABLE`

Reports whether the battery is detected / available.

Value type:
- `TRUE`
- `FALSE`

Examples:

```text
2026-03-27T12:04:00Z,TLM,BATTERY,AVAILABLE,TRUE
2026-03-27T12:04:00Z,TLM,BATTERY,AVAILABLE,FALSE
```

### `BATTERY,TELEMETRY`

Reports whether battery data is included in periodic telemetry snapshots.

Value type:
- `TRUE`
- `FALSE`

Examples:

```text
2026-03-27T12:04:00Z,TLM,BATTERY,TELEMETRY,TRUE
2026-03-27T12:04:00Z,TLM,BATTERY,TELEMETRY,FALSE
```

### `BATTERY,CHARGE_CURRENT_A`

Reports charge current in amps.

Value type:
- floating-point number

Examples:

```text
2026-03-27T12:04:02Z,TLM,BATTERY,CHARGE_CURRENT_A,0.500
2026-03-27T12:04:02Z,TLM,BATTERY,CHARGE_CURRENT_A,0.125
```

### `BATTERY,CHARGE_VOLTAGE_V`

Reports charge voltage in volts.

Value type:
- floating-point number

Examples:

```text
2026-03-27T12:04:03Z,TLM,BATTERY,CHARGE_VOLTAGE_V,4.200
2026-03-27T12:04:03Z,TLM,BATTERY,CHARGE_VOLTAGE_V,3.950
```

### `BATTERY,CHARGE_PERCENT_P`

Reports approximate battery charge percentage.

Value type:
- integer percentage

Examples:

```text
2026-03-27T12:04:04Z,TLM,BATTERY,CHARGE_PERCENT_P,87
2026-03-27T12:04:04Z,TLM,BATTERY,CHARGE_PERCENT_P,42
```

### `BATTERY,VOLTAGE_V`

Reports measured battery voltage in volts.

Value type:
- floating-point number

Examples:

```text
2026-03-27T12:04:05Z,TLM,BATTERY,VOLTAGE_V,4.010
2026-03-27T12:04:05Z,TLM,BATTERY,VOLTAGE_V,3.720
```

---

## Telemetry Parameter Reference

This table is intended for a ground-station developer or future parser implementation.

| Target | Parameter | Meaning | Value Type | Example |
|---|---|---|---|---|
| `LED` | `ENABLE` | Whether LED operation is permitted | `TRUE` / `FALSE` | `2026-03-27T12:02:20Z,TLM,LED,ENABLE,FALSE` |
| `LED` | `TELEMETRY` | Whether LED data is included in periodic snapshots | `TRUE` / `FALSE` | `2026-03-27T12:02:20Z,TLM,LED,TELEMETRY,TRUE` |
| `LED` | `STATE` | Current LED output state | `ON` / `OFF` | `2026-03-27T12:02:20Z,TLM,LED,STATE,OFF` |
| `LED` | `COLOR` | Current selected LED color | `RED` / `GREEN` / `BLUE` | `2026-03-27T12:02:20Z,TLM,LED,COLOR,GREEN` |
| `STATUS` | `HEARTBEAT_N` | Heartbeat counter incremented once per periodic snapshot | unsigned integer | `2026-03-27T12:02:20Z,TLM,STATUS,HEARTBEAT_N,12` |
| `RTC` | `TELEMETRY` | Whether RTC data is included in periodic snapshots | `TRUE` / `FALSE` | `2026-03-27T12:02:20Z,TLM,RTC,TELEMETRY,TRUE` |
| `RTC` | `CURRENT_TIME` | Current device UTC time | ISO UTC string | `2026-03-27T12:02:20Z,TLM,RTC,CURRENT_TIME,2026-03-27T12:02:20Z` |
| `RTC` | `SYNC` | Whether the device clock is in sync | `TRUE` / `FALSE` | `2026-03-27T12:02:20Z,TLM,RTC,SYNC,TRUE` |
| `TELEMETRY` | `TELEMETRY` | Whether telemetry-status data is included in periodic snapshots | `TRUE` / `FALSE` | `2026-03-27T12:02:20Z,TLM,TELEMETRY,TELEMETRY,TRUE` |
| `TELEMETRY` | `ENABLE` | Whether periodic telemetry is enabled | `TRUE` / `FALSE` | `2026-03-27T12:02:20Z,TLM,TELEMETRY,ENABLE,TRUE` |
| `TELEMETRY` | `INTERVAL_S` | Telemetry interval in seconds | unsigned integer | `2026-03-27T12:02:20Z,TLM,TELEMETRY,INTERVAL_S,5` |
| `BATTERY` | `TELEMETRY` | Whether battery data is included in periodic snapshots | `TRUE` / `FALSE` | `2026-03-27T12:02:20Z,TLM,BATTERY,TELEMETRY,TRUE` |
| `BATTERY` | `AVAILABLE` | Whether battery hardware is present | `TRUE` / `FALSE` | `2026-03-27T12:02:20Z,TLM,BATTERY,AVAILABLE,TRUE` |
| `BATTERY` | `CHARGE_CURRENT_A` | Charge current in amps | float | `2026-03-27T12:02:20Z,TLM,BATTERY,CHARGE_CURRENT_A,0.500` |
| `BATTERY` | `CHARGE_VOLTAGE_V` | Charge voltage in volts | float | `2026-03-27T12:02:20Z,TLM,BATTERY,CHARGE_VOLTAGE_V,4.200` |
| `BATTERY` | `CHARGE_PERCENT_P` | Approximate battery percentage | integer | `2026-03-27T12:02:20Z,TLM,BATTERY,CHARGE_PERCENT_P,87` |
| `BATTERY` | `VOLTAGE_V` | Measured battery voltage in volts | float | `2026-03-27T12:02:20Z,TLM,BATTERY,VOLTAGE_V,4.010` |

---

## Useful Ground-Station Decoding Rules

A ground station should parse telemetry as:

- `TIME` = ISO UTC timestamp string
- `TYPE` = must equal `TLM`
- `TARGET` = subsystem key
- `PARAMETER` = attribute key
- `VALUE` = symbolic, integer, or floating-point value

Recommended internal keying model:

```text
(TARGET, PARAMETER) -> latest VALUE at TIME
```

or, if keeping history:

```text
(TIME, TARGET, PARAMETER, VALUE)
```

### Recommended UI mapping

| Telemetry | Suggested UI representation |
|---|---|
| `LED,ENABLE` | enabled/disabled icon or status badge |
| `LED,TELEMETRY` | LED telemetry enabled indicator |
| `LED,STATE` | on/off icon or status badge |
| `LED,COLOR` | color label or swatch |
| `STATUS,HEARTBEAT_N` | heartbeat counter and last-heartbeat freshness indicator |
| `RTC,TELEMETRY` | RTC telemetry enabled indicator |
| `RTC,CURRENT_TIME` | device time display |
| `RTC,SYNC` | clock sync indicator |
| `TELEMETRY,TELEMETRY` | telemetry-status enabled indicator |
| `TELEMETRY,ENABLE` | stream active indicator |
| `TELEMETRY,INTERVAL_S` | numeric display |
| `BATTERY,TELEMETRY` | battery telemetry enabled indicator |
| `BATTERY,AVAILABLE` | battery present indicator |
| `BATTERY,CHARGE_CURRENT_A` | numeric current display |
| `BATTERY,CHARGE_VOLTAGE_V` | numeric voltage display |
| `BATTERY,CHARGE_PERCENT_P` | battery percentage display / gauge |
| `BATTERY,VOLTAGE_V` | measured battery voltage display |

---

## Serial Studio Guidance

Serial Studio will need a parser because the MySat protocol is not simple fixed-column numeric CSV.

A parser should:

1. split the incoming line on commas
2. verify field 2 is `TLM`
3. decode `TARGET`
4. decode `PARAMETER`
5. update the latest known state for widgets

### Example telemetry lines for testing a parser

```text
2026-03-27T12:01:40Z,TLM,RTC,TELEMETRY,TRUE
2026-03-27T12:01:40Z,TLM,RTC,CURRENT_TIME,2026-03-27T12:01:40Z
2026-03-27T12:01:40Z,TLM,RTC,SYNC,TRUE
2026-03-27T12:01:40Z,TLM,LED,ENABLE,FALSE
2026-03-27T12:01:40Z,TLM,LED,STATE,OFF
2026-03-27T12:01:40Z,TLM,LED,COLOR,GREEN
2026-03-27T12:01:50Z,TLM,STATUS,HEARTBEAT_N,7
2026-03-27T12:01:50Z,TLM,TELEMETRY,TELEMETRY,TRUE
2026-03-27T12:01:50Z,TLM,TELEMETRY,ENABLE,TRUE
2026-03-27T12:01:50Z,TLM,TELEMETRY,INTERVAL_S,5
2026-03-27T12:02:00Z,TLM,BATTERY,TELEMETRY,TRUE
2026-03-27T12:02:00Z,TLM,BATTERY,AVAILABLE,TRUE
2026-03-27T12:02:00Z,TLM,BATTERY,CHARGE_CURRENT_A,0.500
2026-03-27T12:02:00Z,TLM,BATTERY,CHARGE_VOLTAGE_V,4.200
2026-03-27T12:02:00Z,TLM,BATTERY,CHARGE_PERCENT_P,87
2026-03-27T12:02:00Z,TLM,BATTERY,VOLTAGE_V,4.010
```

### Suggested Serial Studio widget set

For the current project, useful widgets would be:

- last message timestamp
- last heartbeat time
- heartbeat counter
- RTC current time
- RTC sync state
- LED enabled
- LED state
- LED color
- telemetry telemetry enabled
- telemetry enabled
- telemetry interval seconds
- battery telemetry enabled
- battery available
- charge current
- charge voltage
- charge percentage
- battery voltage
- last ACK
- last ERR

### Suggested stateful parser behaviour

Because telemetry arrives one line at a time, a parser should maintain the latest known values and update the UI state incrementally.

---

## Relationship to Other Message Types

The device also emits:

- `ACK` messages
- `ERR` messages

These are not telemetry, but they use the same timestamp prefix format.

Examples:

```text
2026-03-27T12:02:00Z,ACK,LED,ON
2026-03-27T12:02:01Z,ERR,LED_DISABLED
```

A ground station should treat these separately from telemetry.

---

## Future Growth

The telemetry structure is intentionally reusable.

Future examples may include:

```text
2026-03-27T12:05:00Z,TLM,POWER,MODE,LOW_POWER
2026-03-27T12:05:01Z,TLM,RADIO,STATE,OFF
2026-03-27T12:05:02Z,TLM,STATUS,HEALTH,OK
2026-03-27T12:05:03Z,TLM,UPTIME,SECONDS,303
```

This means the telemetry parser should not be hard-coded only for LED. It should support generic:

```text
TIME,TLM,TARGET,PARAMETER,VALUE
```

with target/parameter-specific UI mapping layered on top.
