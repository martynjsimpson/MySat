# RF Transition

This document now mixes phase-one design intent with the current implemented RF behavior. A cleanup pass can later split the final baseline into `RF.md` and retire this transition-oriented name.

## Purpose

This document records the planned transition from the current serial-first bench setup to an RF-backed architecture using two Arduino MKR WAN boards.

It is intended to be a living design note:

- confirmed decisions should read as decisions, not open questions
- unresolved items should remain visible until they are settled
- deferred work should be captured separately so the phase-one scope stays clear

## Scope

The transition described here assumes:

- the satellite board will move from `MKR WiFi 1010` to `MKR WAN`
- the physical ground station will move from `Arduino Mega 2560` to `MKR WAN`
- the host PC will remain the dashboard host and operator interface
- the ground-station board will act primarily as a LoRa-to-USB serial bridge

The intended runtime topology is:

1. Host PC
2. Ground-station MKR WAN
3. Satellite MKR WAN

In that model:

- the host PC talks to the ground-station board over USB serial
- the ground-station board forwards one logical message at a time over LoRa
- the satellite executes commands and returns `ACK`, `ERR`, and `TLM`
- the ground-station board forwards received radio traffic back to the host PC over USB serial

## Design Goal

The main goal is to preserve the existing logical protocol while changing the physical transport.

That means:

- command semantics should stay stable
- target behavior should stay stable
- dashboard behavior should stay as stable as possible
- transport-specific concerns should be isolated to the radio link and bridge layers

This matches the transport abstraction already introduced in the satellite firmware.

## Phase-One Principles

These are the parts of the current system we are deliberately trying to preserve in phase one:

- the current logical command model: `COMMAND,TARGET,PARAMETER,VALUE`
- the response types (including any additional that exist at the time): `ACK`, `ERR`, `TLM`
- the existing target model (including any additional that exist at the time): `RTC`, `GPS`, `THERMAL`, `IMU`, `ADCS`, `BATTERY`, `TELEMETRY`, `STATUS`
- the host-side serial workflow and dashboard expectations

The intent is for RF to feel like a link substitution rather than a full protocol rewrite.

## RF Constraints

Even if the logical protocol stays the same, the RF link introduces constraints that do not exist on local USB serial:

- lower throughput
- meaningful airtime cost per packet
- likely half-duplex behavior in practice
- contention between commands and periodic telemetry
- greater sensitivity to loss or delay

Because of that, the radio path needs explicit transport policy rather than assuming the link is effectively always available.

## Phase-One Architecture

## Current Implementation Status

The following phase-one RF work is now implemented:

- both firmware targets build for `MKR WAN 1310`
- the satellite transport is LoRa-backed rather than serial-backed
- the ground station is a USB serial to LoRa bridge
- both sides use the shared phase-one RF envelope:
  `[version][source][destination][payload_length][payload][crc16]`
- command retries are owned by the ground station
- the host still sees newline-terminated logical protocol messages over USB serial
- the legacy Node-RED dashboard path has been removed in favour of the `uibuilder` dashboard

The main remaining work is behaviour hardening and documentation cleanup rather than first-pass RF bring-up.

### Ground Station Role

The ground-station MKR WAN should remain a thin bridge in phase one:

- it accepts the existing logical protocol over USB serial from the host PC
- it wraps that payload in the RF envelope
- it transmits and receives RF packets
- it forwards logical payloads back to the host PC over USB serial

The satellite remains the source of truth for protocol behavior and target behavior.

### Host Interface

The host-facing interface should remain unchanged in phase one:

- the host PC continues to talk serial to the ground station
- the host continues to see the existing logical protocol messages
- the bridge should remain transparent rather than introducing a second protocol at the host boundary

## Phase-One RF Envelope

Phase one will use a thin RF envelope around the existing logical protocol messages.

The payload remains the current human-readable text format, for example:

```text
GET,GPS,NONE,NONE
```

or:

```text
2026-03-30T20:12:40Z,TLM,GPS,LATITUDE_D,48.85837
```

The RF envelope, not the text payload, defines packet boundaries.

### Envelope Fields

The phase-one envelope is:

```text
[version][source][destination][payload_length][payload...][crc16_hi][crc16_lo]
```

Field definitions:

- `version`: RF envelope version
- `source`: one-byte sender id
- `destination`: one-byte receiver id
- `payload_length`: one-byte payload length in bytes
- `payload`: exactly one logical protocol message
- `crc16`: RF-layer checksum

### Envelope Decisions

- `version` is `0x01`
- `source` and `destination` are one-byte numeric IDs
- `payload_length` is one byte
- `payload` contains exactly one logical protocol message
- `crc16` uses `CRC-16/CCITT-FALSE`

This gives a fixed envelope overhead of `6` bytes before payload.

No separate RF packet-type field is planned for phase one. Application meaning remains inside the payload itself through the existing `GET`, `SET`, `TLM`, `ACK`, `ERR`, and related message types.

### Addressing

The current phase-one addressing scheme is:

- `0x01` = satellite
- `0x02` = ground station
- `0xFF` = broadcast

The satellite should ignore packets not addressed to the satellite device id.

The ground station should ignore packets not addressed to the ground-station device id, except for any later broadcast behavior if that is ever introduced.

## Message Granularity

Phase one will map one logical protocol message to one RF packet.

Reasons:

- preserves per-field freshness timing in the dashboard
- keeps the bridge simple
- keeps bring-up and debugging easier
- limits packet loss to one logical message rather than a whole bundled snapshot

This means a `GET` command, a single `ACK`, or one `TLM` line each travel as their own RF packet payload.

## Reliability Model

### Retry Ownership

In phase one:

- command retries are owned by the ground-station bridge
- the satellite does not own retry policy
- telemetry is best-effort and is not retried

Expected command flow:

1. the host sends a command to the ground-station bridge
2. the ground station transmits the command over RF
3. the ground station waits for `ACK` or `ERR`
4. if no response arrives in time, the ground station retries the command
5. if an `ACK` or `ERR` arrives, retries stop

This keeps retry ownership on one side and avoids unnecessary state duplication on the satellite in phase one.

### Draft Retry Timing

The current draft timing values are:

- wait `5` seconds for a response before retrying
- retry up to `3` times

These values should be validated against real LoRa behavior once the hardware is available.

## Telemetry Policy

### Routine Telemetry

The current phase-one telemetry scheduler policy is:

- periodic telemetry is `skip-if-busy`
- periodic telemetry is not queued
- missed telemetry cycles are dropped rather than replayed later
- commands, `ACK`, and `ERR` always take priority over routine telemetry

Expected behavior:

1. a telemetry interval arrives
2. the transport checks whether the RF link is ready
3. if the link is ready, the telemetry cycle is sent
4. if the link is busy, that telemetry cycle is skipped
5. the next interval proceeds normally

This avoids stale telemetry backlog and protects command responsiveness.

Current implementation detail:

- the satellite now defers routine periodic telemetry when there is unread inbound RF data
- the satellite also defers routine periodic telemetry for a short quiet window after recent RF activity
- heartbeat-only mode still emits heartbeat on interval even when normal global telemetry is disabled

The current quiet-window value is `250 ms` and should be treated as a tunable bench value rather than a final fixed constant.

### Heartbeat

Heartbeat is retained in the RF design, but it should be treated differently from ordinary telemetry.

Its purpose is to answer the operator question:

- is the satellite alive and are comms up?

LoRa peer-to-peer mode does not provide a strong built-in connected-state concept, so heartbeat still has value as an application-level liveness signal.

The long-term dashboard intent is to expose a comms or liveness indicator driven primarily by:

- heartbeat freshness

and secondarily by:

- recent valid command responses or other valid traffic

Heartbeat is expected to sit above routine telemetry in priority, but that should be validated once the RF link is real.

## Draft RF Traffic Priority

The current draft priority order is:

1. `ERR`
2. `ACK`
3. inbound commands awaiting handling
4. heartbeat / liveness traffic
5. routine periodic telemetry

This ordering should be treated as a draft for real-world validation rather than a fixed truth until the LoRa link is tested.

## Decisions Made

- The satellite will move from `MKR WiFi 1010` to `MKR WAN`.
- The ground station will move from `Arduino Mega 2560` to `MKR WAN`.
- The ground-station board will remain primarily a LoRa-to-USB serial bridge.
- The host PC will remain the dashboard host and operator interface.
- The host-facing interface will remain serial and protocol-compatible in phase one.
- Phase one will use a thin RF envelope around the existing logical protocol messages.
- The payload will remain the existing text protocol format.
- The RF envelope will be length-prefixed.
- Phase one will carry exactly one logical protocol message per RF packet.
- The RF envelope will include source and destination fields.
- Source and destination will be one-byte numeric IDs.
- The current addressing plan is `0x01` satellite, `0x02` ground station, `0xFF` broadcast.
- The satellite will only act on packets addressed to it.
- Phase one will include `CRC-16/CCITT-FALSE`.
- The RF envelope version will be `0x01`.
- Command retries will be owned by the ground-station bridge.
- Telemetry will be best-effort and not retried in phase one.
- Periodic telemetry will use a skip-if-busy policy.
- Heartbeat will be retained as a liveness signal rather than treated as ordinary telemetry.
- Heartbeat is expected to have higher priority than routine telemetry, subject to validation on real hardware.
- The bridge-to-host interface will remain transparent in phase one.

## Open Questions

- What is the maximum acceptable command round-trip latency?

## Future Considerations

These items are intentionally deferred rather than undecided.

- Add shared-key command authentication as the next security phase after the baseline RF transport is stable.
- Add explicit sequence and retry metadata after the baseline RF transport is stable.
- Revisit duplicate suppression once sequence metadata exists.
- Consider bundled multi-field telemetry only if real airtime testing shows one-message-per-packet is too expensive.
- Consider bridge-originated link-health reporting only if the transparent host interface proves insufficient.
- Revisit heartbeat priority and cadence once the real LoRa link is tested.
- If payloads ever grow beyond `255` bytes, introduce a later RF envelope version with a larger payload-length field rather than expanding phase one now.

## Candidate Migration Stages

One reasonable staged approach is:

1. Replace the satellite `MKR WiFi 1010` with an `MKR WAN`.
2. Replace the ground-station `Arduino Mega 2560` with an `MKR WAN`.
3. Keep the existing serial protocol unchanged on the host side.
4. Implement the thin RF envelope described above.
5. Forward one logical protocol message per RF packet.
6. Add CRC validation and address filtering.
7. Add bridge-owned command acknowledgement and timeout handling.
8. Add radio-busy-aware telemetry scheduling.
9. Validate heartbeat behavior and draft traffic priority on real RF hardware.
10. Revisit deferred items only after the base RF link is stable.
