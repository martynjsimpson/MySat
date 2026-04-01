# RF

## Purpose

This document defines the current RF architecture used by MySat.

It describes how the host, ground station, and satellite interact today, what the RF packet format is, and which side owns transport behaviour such as retries and telemetry deferral.

Generic command syntax remains documented in [Protocol.md](./Protocol.md). Generic telemetry behaviour remains documented in [Telemetry.md](./Telemetry.md). Ground-station-local behaviour remains documented in [targets/GROUND.md](./targets/GROUND.md).

## Topology

The current runtime topology is:

1. host PC
2. ground-station `MKR WAN 1310`
3. satellite `MKR WAN 1310`

Physical links:

- host PC to ground station over USB serial
- ground station to satellite over LoRa

Logical message model:

- the host still sends and receives newline-terminated MySat protocol messages
- the ground station wraps satellite-bound messages in the RF envelope
- the satellite unwraps RF packets and feeds the payload into the existing protocol parser

## Node Responsibilities

### Host PC

- runs the Node-RED dashboard or any other host-side serial client
- sends logical protocol commands over USB serial
- receives forwarded satellite traffic and host-local `GROUND` traffic

### Ground Station

- owns the host USB serial boundary
- owns the LoRa link to the satellite
- wraps and unwraps RF packets
- forwards satellite traffic to and from the host
- owns retry and timeout behaviour for satellite-bound commands
- can seed its local clock from trusted satellite timestamps while still `UNSYNC`
- exposes the host-local `GROUND` target for bridge status, counters, heartbeat, telemetry control, clock sync, ping, and reset

### Satellite

- is the source of truth for satellite command execution
- decodes RF packets addressed to the satellite
- executes target behaviour
- emits protocol `ACK`, `ERR`, and `TLM` payloads back over LoRa
- defers routine telemetry when the RF link is busy

## Host Boundary

The host-facing interface remains protocol-compatible.

At the USB serial boundary the host sees:

- forwarded satellite `ACK`, `ERR`, and `TLM` messages unchanged
- host-local ground-station `ACK`, `ERR`, and `TLM` messages using target `GROUND`

There is no separate host-side transport protocol beyond the existing MySat message format.

## RF Packet Envelope

The current RF envelope is:

```text
[version][source][destination][payload_length][timestamp_seconds_be32][payload...][crc16_hi][crc16_lo]
```

Field definitions:

- `version`: RF envelope version byte
- `source`: one-byte sender id
- `destination`: one-byte receiver id
- `payload_length`: one-byte payload length in bytes
- `timestamp_seconds_be32`: packet timestamp as a 4-byte big-endian Unix timestamp
- `payload`: one logical MySat protocol message, or a newline-delimited batch of telemetry lines
- `crc16`: `CRC-16/CCITT-FALSE`

Current fixed values:

- envelope version: `0x02`
- satellite device id: `0x01`
- ground-station device id: `0x02`
- broadcast id: `0xFF`

The envelope overhead is `10` bytes plus payload.

## Message Granularity

Commands still map one logical protocol message to one RF packet. Satellite telemetry is now allowed to batch multiple lines into one RF packet.

Examples:

- one `GET` command line becomes one RF packet
- one `ACK` line becomes one RF packet
- one `ERR` line becomes one RF packet
- one routine telemetry batch for a target can contain several newline-delimited `TLM,...` lines in one RF packet

Ground-station forwarding expands packet-level timestamps and telemetry batches back into normal host-visible `TIME,TLM,...` lines, so the host protocol shape remains unchanged.

## Receive Rules

### Satellite

- accepts only packets addressed to the satellite id
- validates envelope version and CRC
- drops invalid, misaddressed, or oversized packets
- appends a newline to the decoded payload before passing it into the existing parser path

### Ground Station

- accepts only packets addressed to the ground-station id
- validates envelope version and CRC
- drops invalid, misaddressed, or oversized packets
- uses the packet timestamp to reconstruct host-visible timestamps on forwarded lines
- splits valid newline-delimited telemetry batches back into normal host-visible protocol lines

## Command and Response Flow

For satellite-bound traffic:

1. host sends a logical command over USB serial to the ground station
2. ground station wraps the command in the RF envelope and transmits it
3. satellite receives, validates, decodes, and executes the command
4. satellite emits `ACK`, `ERR`, or `TLM` payload lines over LoRa
5. ground station unwraps the RF packet, reconstructs host-visible timestamps from the packet header, and forwards the logical lines to the host

For ground-station-local traffic:

1. host sends a command targeting `GROUND`
2. ground station handles it locally
3. ground station emits protocol-shaped `ACK`, `ERR`, and `TLM` messages directly on USB serial

## Retry and Timeout Policy

The ground station owns retry policy for satellite-bound commands.

Current implemented behaviour:

- wait `3` seconds for a completion response
- retry up to `5` times

Response matching:

- `GET` is satisfied by the first matching `TLM` or any `ERR`
- `SET`, `PING`, and `RESET` are satisfied by `ACK` or `ERR`

Telemetry is best-effort and is not retried.

## Telemetry Policy

Routine telemetry on the satellite is `skip-if-busy`.

Current implemented behaviour:

- periodic telemetry is not queued
- if the RF transport is busy, that telemetry cycle is dropped
- commands and their responses take priority over routine telemetry
- the transport treats unread inbound RF data as busy
- the transport also treats a short quiet window after recent RF activity as busy

Current bench tuning:

- telemetry defer window after RF activity: `250 ms`
- telemetry inter-batch gap during routine snapshots: `15 ms`

## Heartbeat and Liveness

Heartbeat remains the main application-level liveness signal.

Satellite:

- `STATUS,HEARTBEAT_N` continues on the configured telemetry interval
- heartbeat is not removed by global telemetry disable

Ground station:

- `GROUND,HEARTBEAT_N` is emitted on the ground-station heartbeat interval
- it ignores `GROUND,TELEMETRY`
- the rest of the periodic ground-station status snapshot is controlled by `GROUND,TELEMETRY`

The dashboard can therefore treat both `STATUS,HEARTBEAT_N` and `GROUND,HEARTBEAT_N` as freshness indicators for each side of the link.

## Current Tunables

Ground station:

- serial heartbeat interval: `5000 ms`
- command retry delay: `3000 ms`
- max command retries: `5`
- LoRa frequency: `868000000 Hz`
- LoRa TX power: `17 dBm`
- spreading factor: `7`
- bandwidth: `250 kHz`
- coding rate denominator: `5`
- preamble length: `6`
- sync word: `0x12`

Satellite:

- telemetry defer after RF activity: `250 ms`
- telemetry inter-batch gap during routine snapshots: `15 ms`
- LoRa frequency: `868000000 Hz`
- LoRa TX power: `17 dBm`
- spreading factor: `7`
- bandwidth: `250 kHz`
- coding rate denominator: `5`
- preamble length: `6`
- sync word: `0x12`

These are current bench values, not guaranteed final values.

## Related Documents

- [Protocol.md](./Protocol.md)
- [Telemetry.md](./Telemetry.md)
- [targets/GROUND.md](./targets/GROUND.md)
- [RF_Transition.md](./RF_Transition.md)
