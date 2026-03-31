# Electrical Power System (EPS) Concept Note

## Purpose

This note captures the current high-level idea for introducing a simple EPS concept into MySat.

The aim is **not** to build a full spacecraft-grade power system. The aim is to create a realistic, useful, and controllable **bench-top power monitoring model** that fits the rest of the project.

The EPS concept here is primarily about:

- monitoring a dedicated external sensor rail
- separating subsystem power telemetry from the main MCU board power
- creating a path toward future commandable subsystem power control

---

## Core Idea

The Arduino MKR WAN 1310 will continue to be powered normally by USB.

Rather than trying to monitor the board's full USB power path, the INA219 will be used to monitor a **separate 3.3V external subsystem rail**.

That monitored rail will power external devices such as:

- MPU-6050
- QMC5883L
- DHT11
- GPS module
- future sensors or small payload devices

This means the EPS telemetry will represent:

- the voltage available to the external subsystem rail
- the current being drawn by that rail
- the power consumed by that rail

This is much more useful for MySat than trying to measure the whole Arduino board power path.

---

## Why This Approach

Using the INA219 on a separate sensor rail gives several advantages:

- avoids interfering with the MKR USB power path
- makes the measured current easier to interpret
- allows clean separation between **board power** and **subsystem power**
- gives more meaningful housekeeping telemetry
- creates a natural future path to switched subsystem power

This is a good match for a CubeSat-style prototype because it resembles an EPS distributing and monitoring power to subsystems rather than just measuring a single undifferentiated supply.

---

## Hardware Concept

### Main controller
- Arduino MKR WAN 1310

### Power monitor
- INA219 current/voltage monitor breakout

### Power source
- USB power into the MKR WAN 1310

### Monitored rail
- external 3.3V subsystem rail powered from the MKR `3V3` pin through the INA219 measurement path

### Subsystems on monitored rail
Examples:
- IMU
- magnetometer
- temperature/humidity sensor
- GPS
- future external sensors

---

## High-Level Wiring

## INA219 logic connections

These allow the MKR to communicate with the INA219 over I2C.

- INA219 `VCC` -> MKR `3V3`
- INA219 `GND` -> MKR `GND`
- INA219 `SDA` -> MKR `SDA`
- INA219 `SCL` -> MKR `SCL`

## INA219 measurement path

These connections place the INA219 in series with the positive supply feeding the external subsystem rail.

- MKR `3V3` -> INA219 `VIN+`
- INA219 `VIN-` -> external **sensor power rail**
- external sensor VCC pins -> sensor power rail
- external sensor grounds -> common ground rail
- common ground rail -> MKR `GND`

So the measured power path is:

```text
MKR 3V3 -> INA219 VIN+ -> INA219 shunt path -> INA219 VIN- -> external sensor rail -> sensors
```

---

## Wiring Intent

This wiring means:

- the MKR itself is still powered normally from USB
- the INA219 measures only the external subsystem rail
- the telemetry reflects what the external sensor/payload side is doing
- adding or removing external sensors will directly affect the measured values

This makes the EPS telemetry meaningful and easy to reason about.

---

## Future Expansion Idea

A natural future extension is to place a MOSFET-controlled switch on the external subsystem rail.

That would allow the firmware to:

- enable or disable the external sensor rail
- observe current draw before and after enabling the rail
- model simple subsystem power distribution behaviour

This is **not required for the first version**, but the current INA219 rail-monitoring approach is designed to make that extension easy later.

---

## Telemetry Items

For the first EPS implementation, the most useful telemetry items are:

| Target | Parameter | Meaning | Example |
|---|---|---|---|
| `POWER` or `EPS` | `BUS_VOLTAGE_V` | Voltage on the monitored subsystem rail | `TLM,POWER,BUS_VOLTAGE_V,3.28` |
| `POWER` or `EPS` | `CURRENT_A` | Current being drawn by the monitored subsystem rail | `TLM,POWER,CURRENT_A,0.084` |
| `POWER` or `EPS` | `POWER_W` | Calculated power being consumed by the monitored subsystem rail | `TLM,POWER,POWER_W,0.275` |

### Recommended starting set
- `BUS_VOLTAGE_V`
- `CURRENT_A`
- `POWER_W`

This is enough to make the EPS concept useful without overcomplicating the implementation.

---

## Suggested Naming

Either of these targets would be reasonable:

- `POWER`
- `EPS`

### Recommendation
Use `POWER` first if you want the naming to stay simple and obvious.

Use `EPS` later if the subsystem grows into something more explicitly spacecraft-like.

---

## Practical Notes

- This design assumes the external sensor current draw remains within what the MKR 3.3V rail can safely supply.
- All grounds must remain common.
- This is intended as a prototype monitoring setup on a breadboard, not a final flight-like power architecture.
- The main value of this design is observability and future extensibility.

---

## Summary

The current EPS concept for MySat is:

- keep the MKR powered by USB
- use an INA219 to monitor a separate external 3.3V subsystem rail
- power external sensors from that monitored rail
- expose three key telemetry values:
  - `BUS_VOLTAGE_V`
  - `CURRENT_A`
  - `POWER_W`

This provides a realistic and useful first step toward a small spacecraft-style power subsystem without adding unnecessary complexity.
