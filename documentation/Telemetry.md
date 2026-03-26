# MySat Telemetry Specification

## Purpose

This document describes the outbound telemetry format used by MySat and provides practical guidance for decoding it in a future ground station or tools such as Serial Studio.

---

## Telemetry Line Format

All telemetry lines follow this structure:

```text
TIME,TLM,TARGET,PARAMETER,VALUE
```

### Field meanings

| Position | Field | Meaning |
|---|---|---|
| 1 | `TIME` | Elapsed seconds since device boot |
| 2 | `TLM` | Message type identifier for telemetry |
| 3 | `TARGET` | The subsystem or domain being reported |
| 4 | `PARAMETER` | The property being reported |
| 5 | `VALUE` | The current value of that property |

### Example lines

```text
140,TLM,LED,ENABLE,FALSE
140,TLM,LED,STATE,OFF
140,TLM,TELEMETRY,ENABLE,TRUE
140,TLM,TELEMETRY,INTERVAL_S,5
```

---

## Timestamp Rules

`TIME` is:

- integer seconds since boot
- shared by all messages emitted in the same second
- not wall-clock time
- not a real Unix epoch timestamp

### Important interpretation rule

Within a single timestamp, telemetry must not contain multiple conflicting values for the same `TARGET + PARAMETER` pair.

This is acceptable:

```text
140,TLM,LED,ENABLE,FALSE
140,TLM,LED,STATE,OFF
```

This is not acceptable:

```text
140,TLM,LED,STATE,OFF
140,TLM,LED,STATE,ON
```

because the ground station would not know which one is newer.

---

## Telemetry Snapshot Concept

Periodic telemetry is emitted as a **snapshot**.

A telemetry snapshot is the set of telemetry lines emitted together at one timestamp to represent the current device state.

At present, the periodic snapshot includes:

- LED status
- telemetry configuration/status

Typical snapshot:

```text
200,TLM,LED,ENABLE,TRUE
200,TLM,LED,STATE,ON
200,TLM,TELEMETRY,ENABLE,TRUE
200,TLM,TELEMETRY,INTERVAL_S,5
```

---

## Current Targets and Parameters

## LED target

### `LED,ENABLE`

Reports whether LED operation is currently permitted.

Value type:
- `TRUE`
- `FALSE`

Examples:

```text
220,TLM,LED,ENABLE,TRUE
220,TLM,LED,ENABLE,FALSE
```

### `LED,STATE`

Reports whether the LED output is currently on or off.

Value type:
- `ON`
- `OFF`

Examples:

```text
221,TLM,LED,STATE,ON
221,TLM,LED,STATE,OFF
```

---

## TELEMETRY target

### `TELEMETRY,ENABLE`

Reports whether periodic telemetry streaming is enabled.

Value type:
- `TRUE`
- `FALSE`

Examples:

```text
230,TLM,TELEMETRY,ENABLE,TRUE
230,TLM,TELEMETRY,ENABLE,FALSE
```

### `TELEMETRY,INTERVAL_S`

Reports the periodic telemetry interval in seconds.

Value type:
- unsigned integer

Examples:

```text
231,TLM,TELEMETRY,INTERVAL_S,5
231,TLM,TELEMETRY,INTERVAL_S,10
```

---

## Telemetry Parameter Reference

This table is intended for a ground-station developer or future parser implementation.

| Target      | Parameter    | Meaning                               | Value Type       | Example                          |
| ----------- | ------------ | ------------------------------------- | ---------------- | -------------------------------- |
| `LED`       | `ENABLE`     | Whether LED operation is permitted    | `TRUE` / `FALSE` | `140,TLM,LED,ENABLE,FALSE`       |
| `LED`       | `STATE`      | Current LED output state              | `ON` / `OFF`     | `140,TLM,LED,STATE,OFF`          |
| `TELEMETRY` | `ENABLE`     | Whether periodic telemetry is enabled | `TRUE` / `FALSE` | `140,TLM,TELEMETRY,ENABLE,TRUE`  |
| `TELEMETRY` | `INTERVAL_S` | Telemetry interval in seconds         | unsigned integer | `140,TLM,TELEMETRY,INTERVAL_S,5` |

---

## Useful Ground-Station Decoding Rules

A ground station should parse telemetry as:

- `TIME` = integer timestamp
- `TYPE` = must equal `TLM`
- `TARGET` = subsystem key
- `PARAMETER` = attribute key
- `VALUE` = symbolic or numeric value

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
| `LED,STATE` | on/off icon or status badge |
| `TELEMETRY,ENABLE` | stream active indicator |
| `TELEMETRY,INTERVAL_S` | numeric display |

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
100,TLM,LED,ENABLE,FALSE
100,TLM,LED,STATE,OFF
105,TLM,LED,ENABLE,TRUE
105,TLM,LED,STATE,ON
110,TLM,TELEMETRY,ENABLE,TRUE
110,TLM,TELEMETRY,INTERVAL_S,5
```

### Suggested Serial Studio widget set

For the current project, useful widgets would be:

- last timestamp
- LED enabled
- LED state
- telemetry enabled
- telemetry interval seconds
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
120,ACK,LED,ON
121,ERR,LED_DISABLED
```

A ground station should treat these separately from telemetry.

---

## Future Growth

The telemetry structure is intentionally reusable.

Future examples may include:

```text
300,TLM,POWER,MODE,LOW_POWER
301,TLM,RADIO,STATE,OFF
302,TLM,STATUS,HEALTH,OK
303,TLM,UPTIME,SECONDS,303
```

This means the telemetry parser should not be hard-coded only for LED. It should support generic:

```text
TIME,TLM,TARGET,PARAMETER,VALUE
```

with target/parameter-specific UI mapping layered on top.
