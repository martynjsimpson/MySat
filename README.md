# MySat

## Project Purpose

MySat is a hypothetical CubeSat-style embedded project used to explore protocol design, telemetry, command handling, and ground-station concepts using an Arduino-based target.

At the current stage, the project is intentionally simple:

- an Arduino acts as the "satellite"
- a serial link is used first before introducing RF
- the built-in LED is used as the first controllable subsystem
- battery / PMIC telemetry is being added as a real subsystem
- periodic telemetry is emitted
- the protocol is being shaped so it can scale to additional subsystems later

The aim is not only to make the current demo work, but to establish patterns that will still be sensible when the project grows.

---

## Build System

This project now uses **PlatformIO** rather than the Arduino IDE.

That means:

- `platformio.ini` is the main build configuration
- source files live in `src/`
- header files live in `include/`
- external libraries are declared in `platformio.ini` via `lib_deps`

Current PlatformIO environment:

```ini
[env:mkrwifi1010]
platform = atmelsam
board = mkrwifi1010
framework = arduino
monitor_speed = 115200

lib_deps =
    arduino-libraries/RTCZero
    arduino-libraries/Arduino_BQ24195
```

### Typical PlatformIO commands

Build:

```bash
platformio run --environment mkrwifi1010
```

Upload:

```bash
platformio run --target upload --environment mkrwifi1010
```

Serial monitor:

```bash
platformio device monitor --environment mkrwifi1010
```

---

## Current Architecture

The project is now structured as a multi-file PlatformIO project.

### Root
- `platformio.ini` — PlatformIO project configuration
- `README.md` — project overview
- `documentation/` — protocol and telemetry documentation

### `include/`
- `commands.h` — enums and `Command` struct
- `rtc.h` — RTC interface
- `sender.h` — outbound sender interface
- `led.h` — LED subsystem interface
- `pmic.h` — battery / PMIC subsystem interface
- `telemetry.h` — telemetry scheduler / reporting interface
- `protocol.h` — command parser / dispatcher interface

### `src/`
- `main.cpp` — top-level setup and loop
- `rtc.cpp` — RTC implementation
- `sender.cpp` — timestamped message sending implementation
- `led.cpp` — LED subsystem implementation
- `pmic.cpp` — battery / PMIC implementation
- `telemetry.cpp` — telemetry scheduler and snapshot logic
- `protocol.cpp` — command parsing and dispatch implementation

---

## Core Concepts

## 1. Transport vs protocol

The current implementation uses serial, but the protocol itself is intended to be independent of transport.

That means:

- the Arduino should parse the same logical command whether it arrived via USB serial or radio
- the ground station should interpret the same outbound message structure regardless of transport

Serial is just the first transport layer.

## 2. Commands vs telemetry

The project distinguishes clearly between:

- **commands** — requests sent to the device
- **telemetry** — state reported by the device
- **acknowledgements** — confirmation that a command was accepted
- **errors** — notification that something was rejected or malformed

### Inbound command structure

```text
COMMAND,TARGET,PARAMETER,VALUE
```

### Outbound response structure

```text
TIME,TYPE,...
```

Where `TYPE` is:
- `ACK`
- `ERR`
- `TLM`

This separation is intentional and should be preserved.

## 3. Policy state vs hardware state

A key design idea in the current code is that some states are **policy/control states** and some are **measured or actual states**.

Example for the LED:

- LED enablement is policy state
- LED output state is read from hardware

This distinction is important.

### Example

The LED can be:
- enabled and off
- enabled and on
- disabled and off

But it must never be:
- disabled and on

The code should continue to model policy and actual state separately where appropriate.

## 4. Parameterised commands

The project uses:

```text
COMMAND,TARGET,PARAMETER,VALUE
```

This is the correct pattern and should be used going forward.

Examples:

```text
SET,LED,STATE,ON
SET,LED,ENABLE,TRUE
SET,TELEMETRY,INTERVAL_S,5
GET,BATTERY,NONE,NONE
```

---

## Development Standards

## 1. Prefer structured protocols over sentence-like commands

Good:

```text
SET,LED,STATE,ON
```

Bad:

```text
TURN THE LED ON
```

The protocol should stay machine-oriented and regular.

## 2. Keep parsing separate from behaviour

The code should continue to follow this pattern:

1. read a full command line
2. split into fields
3. parse into typed internal representation
4. dispatch to target-specific handlers
5. emit structured responses

Avoid mixing string parsing directly into subsystem logic.

## 3. Use small handlers

Subsystem logic should be kept in focused handlers rather than giant monolithic functions.

Examples:
- `handleSetLed(...)`
- `reportLedStatus()`
- `handleSetTelemetry(...)`
- `reportTelemetryStatus()`
- `reportBatteryStatus()`

This pattern should continue as the project grows.

## 4. Keep outbound messages structured and machine-readable

Responses should stay regular and never drift into natural-language output.

Good:

```text
20,ACK,LED,ON
21,ERR,BAD_VALUE
22,TLM,LED,STATE,OFF
```

Bad:

```text
The LED is now off
There was an error with your command
```

## 5. Preserve timestamped outputs

All outbound lines should continue to be timestamp-prefixed.

This makes:
- logging easier
- debugging easier
- ground-station parsing easier

## 6. Avoid ambiguous state naming

Use tokens consistently:

- `ENABLE` / `TRUE` / `FALSE` for enablement and boolean control
- `STATE` / `ON` / `OFF` for state-like outputs
- `INTERVAL_S` for second-based intervals
- explicit telemetry parameter names such as `AVAILABLE`, `ON_BATTERY`, `CHARGE_CURRENT_A`

Do not casually rename these without updating protocol docs and parser logic.

## 7. Keep protocol docs authoritative

The docs should remain the source of truth for:
- command shapes
- response formats
- token meanings
- telemetry semantics

Any protocol change should update documentation alongside code.

---

## Current Implemented Features

### LED subsystem
- enable/disable control
- on/off state control
- telemetry reporting

### Telemetry subsystem
- enable/disable periodic telemetry
- configurable telemetry interval in seconds
- telemetry status reporting

### Battery / PMIC subsystem
- battery availability telemetry
- on-battery telemetry
- charge current telemetry
- charge voltage telemetry
- charge percentage telemetry

### Protocol
- 4-field command format
- timestamped `ACK`, `ERR`, and `TLM`
- numeric interval setting
- placeholder `NONE` values for non-applicable fields

---

## Current Behavioural Rules

### LED rules
- LED cannot be turned on unless enabled
- disabling LED also forces LED off
- LED telemetry reports both enable state and output state

### Telemetry rules
- periodic telemetry is enabled by default
- default interval is 5 seconds
- telemetry interval is configurable through command
- telemetry snapshots must not emit conflicting values for the same target/parameter pair at the same timestamp

### Battery rules
- battery telemetry is currently read-only
- battery telemetry is included in periodic telemetry snapshots
- battery telemetry can be requested on demand with `GET,BATTERY,NONE,NONE`

---

## Guidance for Future Development

## Adding a new subsystem

When adding a new subsystem:

1. add a new `TARGET_*`
2. add any needed `PARAM_*`
3. add any needed `VALUE_*`
4. extend target parsing
5. implement subsystem handlers
6. implement telemetry reporting
7. update documentation

### Example future subsystem candidates
- radio
- power
- thermal
- payload
- watchdog
- status/health

## Adding a new command
Prefer reusing:
- `SET`
- `GET`

Reserve new commands for genuinely different semantics such as:
- `RESET`
- `SAVE`

Do not create new verbs unnecessarily if the behaviour fits as a `SET` against a target/parameter/value combination.

## Adding numeric settings
If a setting is numeric, keep using the current 4-field structure and interpret the final field as numeric when appropriate.

Example:

```text
SET,TELEMETRY,INTERVAL_S,10
```

Do not try to encode numeric configuration into enum-only symbolic values.

## Ground station expectations
A future ground station should assume:

- commands are explicit and structured
- responses are timestamped
- telemetry is stateful and incremental
- multiple telemetry lines may share a timestamp
- but the same `TARGET + PARAMETER` must not produce conflicting values within that timestamp

---

## AI / Future Helper Notes

If an AI assistant or future developer works on this project, it should preserve these assumptions:

1. The current protocol shape is intentional.
2. `COMMAND,TARGET,PARAMETER,VALUE` is the correct inbound model.
3. `TIME,TYPE,...` is the correct outbound model.
4. Policy state and hardware state are not the same thing.
5. Responses should remain machine-readable.
6. Documentation changes should accompany protocol changes.
7. The project is built with PlatformIO, not the Arduino IDE.

The project should evolve by extending the structure, not by bypassing it with ad hoc shortcuts.

---

## Practical Testing Mindset

The preferred development order is:

1. make the protocol work over serial
2. verify behaviour in a serial monitor
3. verify telemetry parsing in a basic ground station
4. only then move the same protocol across RF

This reduces complexity and keeps bugs easier to isolate.

---

## Summary

MySat is currently a small but intentionally structured embedded command-and-telemetry project.

The most important idea is this:

**build the simple version in a way that still makes sense when the project gets bigger.**
