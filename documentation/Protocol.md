# MySat Communication Protocol

## Purpose

This document defines the command and response protocol used by the MySat Arduino-based CubeSat simulator.

The protocol is currently implemented over a serial connection for development and test, but it is designed to be transport-agnostic so the same logical protocol can later be carried over RF.

Telemetry-specific field definitions and decoding guidance live in [Telemetry.md](./Telemetry.md).

---

## Design Goals

The protocol is designed to be:

- simple to type by hand during development
- easy to parse on embedded hardware
- structured enough to scale beyond the LED demo
- readable in logs and serial consoles
- suitable for a future ground-station parser

---

## Command Structure

Commands sent **to the device** use a fixed four-field comma-separated structure:

```text
COMMAND,TARGET,PARAMETER,VALUE
```

Each command is terminated by a newline.

### Field meanings

| Position | Field | Purpose |
|---|---|---|
| 1 | `COMMAND` | The action to perform |
| 2 | `TARGET` | The subsystem, device, or command domain being addressed |
| 3 | `PARAMETER` | The property or aspect of the target being acted on |
| 4 | `VALUE` | The symbolic or numeric value to apply |

### Examples

```text
SET,LED,ENABLE,TRUE
SET,LED,ENABLE,FALSE
SET,LED,TELEMETRY,ENABLE
SET,LED,TELEMETRY,DISABLE
SET,LED,STATE,ON
SET,LED,STATE,OFF
SET,TELEMETRY,ENABLE,TRUE
SET,TELEMETRY,ENABLE,FALSE
SET,TELEMETRY,INTERVAL_S,5
SET,BATTERY,TELEMETRY,ENABLE
SET,BATTERY,TELEMETRY,DISABLE
GET,LED,NONE,NONE
GET,TELEMETRY,NONE,NONE
GET,BATTERY,NONE,NONE
PING,NONE,NONE,NONE
```

---

## Response Structure

All lines sent **from the device** are timestamped.

General outbound format:

```text
TIME,TYPE,...
```

Where:

- `TIME` = uptime formatted as `hh:mm:ss` since device boot
- `TYPE` = `ACK`, `ERR`, or `TLM`

### Timestamp meaning

The device uses the onboard RTC and sets its internal time base to zero during startup.

The first field in every outbound message is therefore:

- not wall-clock time
- not a real Unix timestamp
- boot-relative uptime in `hh:mm:ss`

This should be interpreted as:

- mission elapsed time
- boot-relative time
- uptime since startup

---

## Response Types

### ACK

Acknowledgement lines indicate that a command was accepted and applied.

Format:

```text
TIME,ACK,TARGET,VALUE
```

Examples:

```text
00:00:12,ACK,LED,ENABLE
00:00:13,ACK,LED,ON
00:00:14,ACK,LED,RED
00:00:20,ACK,TELEMETRY,INTERVAL_S
00:00:25,ACK,PING,PONG
```

### ERR

Error lines indicate that a command was rejected, malformed, or invalid.

Format:

```text
TIME,ERR,ERROR_CODE[,ENCODED_COMMAND]
```

If the error was triggered while handling an inbound command, the device may append a fourth field containing the original command line in percent-encoded form.

Encoding rules:

- commas are encoded as `%2C`
- percent signs are encoded as `%25`
- other non-alphanumeric separator characters are percent-encoded as needed

This keeps the outbound message safely comma-delimited while still preserving the triggering input for logs or a ground station UI.

Examples:

```text
00:00:30,ERR,BAD_FORMAT
00:00:31,ERR,BAD_VALUE,SET%2CLED%2CSTATE%2CBLINK
00:00:32,ERR,BAD_PARAMETER,SET%2CLED%2CBRIGHTNESS%2C10
00:00:33,ERR,UNKNOWN_TARGET,GET%2CRADIO%2CNONE%2CNONE
00:00:34,ERR,LED_DISABLED,SET%2CLED%2CSTATE%2CON
```

### TLM

Telemetry lines report current subsystem state.

Format:

```text
TIME,TLM,TARGET,PARAMETER,VALUE
```

Examples:

```text
00:00:40,TLM,LED,ENABLE,FALSE
00:00:40,TLM,LED,TELEMETRY,TRUE
00:00:40,TLM,LED,STATE,OFF
00:00:40,TLM,LED,COLOR,GREEN
00:00:40,TLM,TELEMETRY,ENABLE,TRUE
00:00:40,TLM,TELEMETRY,INTERVAL_S,5
00:00:40,TLM,BATTERY,TELEMETRY,TRUE
00:00:40,TLM,BATTERY,AVAILABLE,TRUE
00:00:40,TLM,BATTERY,ON_BATTERY,FALSE
00:00:40,TLM,BATTERY,CHARGE_CURRENT_A,0.500
00:00:40,TLM,BATTERY,CHARGE_VOLTAGE_V,4.200
00:00:40,TLM,BATTERY,CHARGE_PERCENT_P,87
```

---

## Command Types

### Implemented command types

| Token | Meaning |
|---|---|
| `SET` | Apply a value to a target/parameter |
| `GET` | Request telemetry/status for a target |
| `PING` | Confirm parser and communications path are working |

### Reserved command types

| Token | Intended future meaning |
|---|---|
| `RESET` | Reset a subsystem or service |
| `SAVE` | Persist configuration or logs |

---

## Target Types

### Implemented targets

| Token | Meaning |
|---|---|
| `LED` | Built-in LED subsystem for development and protocol testing |
| `TELEMETRY` | Telemetry stream control/configuration target |
| `BATTERY` | Battery / PMIC telemetry target |
| `NONE` | Placeholder target when not applicable |

### Reserved targets

| Token | Intended future meaning |
|---|---|
| `MODE` | Spacecraft operating mode |
| `STATUS` | General health or summary status |
| `RADIO` | Communications subsystem |
| `POWER` | Power domain or EPS-like functions |
| `PAYLOAD` | Payload subsystem |
| `THERMAL` | Thermal subsystem |
| `LOG` | Logs or event store |
| `WATCHDOG` | Watchdog or fault-monitoring subsystem |
| `UPTIME` | System uptime reporting |

---

## Parameter Types

### Implemented / defined parameters

| Token | Meaning |
|---|---|
| `ENABLE` | Whether a feature or subsystem is allowed to operate |
| `TELEMETRY` | Whether periodic telemetry for a target is included in snapshots |
| `STATE` | Current state of a target |
| `COLOR` | Selected symbolic color for a target |
| `INTERVAL_S` | Interval in seconds |
| `NONE` | Placeholder parameter when not applicable |

### Telemetry-only parameters currently emitted

| Token | Meaning |
|---|---|
| `AVAILABLE` | Whether battery hardware is detected / available |
| `ON_BATTERY` | Whether the system is currently running from battery |
| `CHARGE_CURRENT_A` | Charge current in amps |
| `CHARGE_VOLTAGE_V` | Charge voltage in volts |
| `CHARGE_PERCENT_P` | Approximate battery charge percentage |

### Reserved parameters

| Token | Intended future meaning |
|---|---|
| `MODE` | Current operating mode |
| `HEALTH` | Health/diagnostic condition |
| `SECONDS` | Elapsed seconds value |

---

## Value Types

Values may be **symbolic** or **numeric**.

### Symbolic values

| Token | Meaning |
|---|---|
| `TRUE` | Boolean true / enabled condition |
| `FALSE` | Boolean false / disabled condition |
| `ENABLE` | Enable a telemetry-related setting |
| `DISABLE` | Disable a telemetry-related setting |
| `ON` | Powered or active output state |
| `OFF` | Powered-down or inactive output state |
| `RED` | Red color selection |
| `GREEN` | Green color selection |
| `BLUE` | Blue color selection |
| `NONE` | Placeholder value |

### Reserved symbolic values

| Token | Intended future meaning |
|---|---|
| `SAFE` | Safe mode |
| `NORMAL` | Nominal operating mode |
| `LOW_POWER` | Reduced power mode |
| `ACTIVE` | Running/active state |
| `IDLE` | Idle/waiting state |
| `OK` | Healthy/nominal status |
| `FAIL` | Fault/failure status |

### Numeric values

Numeric values are currently used for command parameters that take an integer rather than a symbolic token.

Example:

```text
SET,TELEMETRY,INTERVAL_S,5
```

In this case:

- `COMMAND` = `SET`
- `TARGET` = `TELEMETRY`
- `PARAMETER` = `INTERVAL_S`
- `VALUE` = numeric integer `5`

Telemetry values may also be numeric or floating-point.

Examples:

```text
00:00:40,TLM,TELEMETRY,INTERVAL_S,5
00:00:40,TLM,BATTERY,CHARGE_CURRENT_A,0.500
00:00:40,TLM,BATTERY,CHARGE_VOLTAGE_V,4.200
00:00:40,TLM,BATTERY,CHARGE_PERCENT_P,87
```

---

## Usage Guidance

### When to use `ENABLE` vs `STATE`

Use `ENABLE` when controlling whether something is permitted to operate.

Examples:

```text
SET,LED,ENABLE,TRUE
SET,TELEMETRY,ENABLE,FALSE
```

Use `STATE` when controlling or reporting the current operating/output state.

Examples:

```text
SET,LED,STATE,ON
TLM,LED,STATE,OFF
```

### When to use `COLOR`

Use `COLOR` when controlling or reporting a selected color.

Examples:

```text
SET,LED,COLOR,RED
TLM,LED,COLOR,GREEN
```

### When to use `TELEMETRY`

Use `TELEMETRY` when enabling or disabling periodic telemetry for an individual target.

Examples:

```text
SET,LED,TELEMETRY,DISABLE
SET,BATTERY,TELEMETRY,ENABLE
TLM,LED,TELEMETRY,FALSE
```

### When to use `TRUE` / `FALSE`

Use `TRUE` / `FALSE` for boolean conditions.

Examples:

```text
SET,LED,ENABLE,TRUE
TLM,LED,ENABLE,FALSE
TLM,BATTERY,AVAILABLE,TRUE
```

### When to use `ON` / `OFF`

Use `ON` / `OFF` for power-like or state-like conditions.

Examples:

```text
SET,LED,STATE,ON
TLM,LED,STATE,OFF
```

### When to use `NONE`

Use `NONE` only as a placeholder when a field is required by the protocol structure but has no meaningful content.

Examples:

```text
GET,LED,NONE,NONE
GET,TELEMETRY,NONE,NONE
GET,BATTERY,NONE,NONE
PING,NONE,NONE,NONE
```

---

## Implemented Behaviour

## LED target

The LED subsystem models two separate concepts:

1. **Enable state** — whether LED operation is permitted
2. **Output state** — whether the LED is currently driven on or off

### Supported LED commands

```text
SET,LED,ENABLE,TRUE
SET,LED,ENABLE,FALSE
SET,LED,TELEMETRY,ENABLE
SET,LED,TELEMETRY,DISABLE
SET,LED,STATE,ON
SET,LED,STATE,OFF
SET,LED,COLOR,RED
SET,LED,COLOR,GREEN
SET,LED,COLOR,BLUE
GET,LED,NONE,NONE
```

### LED rules

| Command | Behaviour |
|---|---|
| `SET,LED,ENABLE,TRUE` | Allows LED operation |
| `SET,LED,ENABLE,FALSE` | Disables LED operation and forces LED off |
| `SET,LED,TELEMETRY,ENABLE` | Includes LED in periodic telemetry snapshots |
| `SET,LED,TELEMETRY,DISABLE` | Omits LED from periodic telemetry snapshots |
| `SET,LED,STATE,ON` | Turns LED on, but only if enabled |
| `SET,LED,STATE,OFF` | Turns LED off |
| `SET,LED,COLOR,RED` | Selects red color output |
| `SET,LED,COLOR,GREEN` | Selects green color output |
| `SET,LED,COLOR,BLUE` | Selects blue color output |
| `GET,LED,NONE,NONE` | Returns current LED telemetry |

The invalid state combination **disabled + on** should never occur.

## TELEMETRY target

The telemetry subsystem currently controls periodic telemetry streaming.

Subsystem telemetry can also be enabled or disabled per target using commands such as `SET,LED,TELEMETRY,DISABLE`.

### Supported telemetry commands

```text
SET,TELEMETRY,ENABLE,TRUE
SET,TELEMETRY,ENABLE,FALSE
SET,TELEMETRY,INTERVAL_S,5
GET,TELEMETRY,NONE,NONE
```

### Telemetry rules

| Command | Behaviour |
|---|---|
| `SET,TELEMETRY,ENABLE,TRUE` | Enables periodic telemetry |
| `SET,TELEMETRY,ENABLE,FALSE` | Disables periodic telemetry |
| `SET,TELEMETRY,INTERVAL_S,n` | Sets telemetry interval in seconds |
| `GET,TELEMETRY,NONE,NONE` | Returns telemetry configuration/status |

## BATTERY target

The battery / PMIC subsystem is currently read-only from the command side.

### Supported battery commands

```text
GET,BATTERY,NONE,NONE
SET,BATTERY,TELEMETRY,ENABLE
SET,BATTERY,TELEMETRY,DISABLE
```

### Battery rules

| Command | Behaviour |
|---|---|
| `GET,BATTERY,NONE,NONE` | Returns current battery telemetry |
| `SET,BATTERY,TELEMETRY,ENABLE` | Includes battery data in periodic telemetry snapshots |
| `SET,BATTERY,TELEMETRY,DISABLE` | Omits battery data from periodic telemetry snapshots |

Battery telemetry is also included in the periodic telemetry snapshot.

---

## Error Codes

| Error Code | Meaning |
|---|---|
| `BAD_FORMAT` | Command did not match expected field structure |
| `BAD_VALUE` | Value was invalid for the given command/target/parameter |
| `BAD_PARAMETER` | Parameter was invalid for the given target |
| `UNKNOWN_CMD` | Command token was not recognised |
| `UNKNOWN_TARGET` | Target token was not recognised |
| `LED_DISABLED` | LED was commanded on while disabled |
| `OVERFLOW` | Input buffer overflowed before newline was received |

---

## Current Implemented Scope

Currently implemented:

```text
SET,LED,ENABLE,TRUE
SET,LED,ENABLE,FALSE
SET,LED,TELEMETRY,ENABLE
SET,LED,TELEMETRY,DISABLE
SET,LED,STATE,ON
SET,LED,STATE,OFF
SET,LED,COLOR,RED
SET,LED,COLOR,GREEN
SET,LED,COLOR,BLUE
SET,BATTERY,TELEMETRY,ENABLE
SET,BATTERY,TELEMETRY,DISABLE
SET,TELEMETRY,ENABLE,TRUE
SET,TELEMETRY,ENABLE,FALSE
SET,TELEMETRY,INTERVAL_S,<integer>
GET,LED,NONE,NONE
GET,TELEMETRY,NONE,NONE
GET,BATTERY,NONE,NONE
PING,NONE,NONE,NONE
```

---

## Example Session

Commands sent to device:

```text
PING,NONE,NONE,NONE
SET,LED,ENABLE,TRUE
SET,LED,STATE,ON
SET,LED,TELEMETRY,DISABLE
SET,LED,COLOR,RED
GET,LED,NONE,NONE
SET,TELEMETRY,INTERVAL_S,10
GET,TELEMETRY,NONE,NONE
GET,BATTERY,NONE,NONE
```

Possible responses:

```text
00:00:00,ACK,PING,PONG
00:00:01,ACK,LED,ENABLE
00:00:02,ACK,LED,ON
00:00:03,ACK,LED,TELEMETRY_DISABLE
00:00:03,TLM,LED,TELEMETRY,FALSE
00:00:04,ACK,LED,RED
00:00:05,TLM,LED,TELEMETRY,FALSE
00:00:05,TLM,LED,ENABLE,TRUE
00:00:05,TLM,LED,STATE,ON
00:00:05,TLM,LED,COLOR,RED
00:00:06,ACK,TELEMETRY,INTERVAL_S
00:00:07,TLM,TELEMETRY,ENABLE,TRUE
00:00:07,TLM,TELEMETRY,INTERVAL_S,10
00:00:08,TLM,BATTERY,TELEMETRY,TRUE
00:00:08,TLM,BATTERY,AVAILABLE,TRUE
00:00:08,TLM,BATTERY,ON_BATTERY,FALSE
00:00:08,TLM,BATTERY,CHARGE_CURRENT_A,0.500
00:00:08,TLM,BATTERY,CHARGE_VOLTAGE_V,4.200
00:00:08,TLM,BATTERY,CHARGE_PERCENT_P,87
```
