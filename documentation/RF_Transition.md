# RF Transition

## Purpose

This document now tracks what is not yet settled in the RF work.

Current implemented RF behaviour is documented in [RF.md](./RF.md). This file is for remaining transition items, open questions, and deliberately deferred future RF ideas.

## Remaining Transition Work

The basic phase-one RF link is up and running. The remaining transition work is mainly bench validation, tuning, and cleanup.

Current active items:

- validate retry timing on real hardware and tune it if `5 s / 3 retries` feels wrong in practice
- validate heartbeat cadence and traffic priority under longer bench runs
- decide how much ground-station diagnostic output should remain enabled by default
- do a final cleanup pass that retires this transition-oriented document name once the RF baseline feels stable

## Open Questions

- What is the maximum acceptable command round-trip latency for the system?
- Does heartbeat need stronger priority than it has now once the link is exercised under heavier traffic?
- Are the current bench LoRa settings the right long-term default, or should range/airtime tradeoffs push different values?

## Deferred Future RF Work

These items are intentionally out of the current baseline scope.

- Add shared-key command authentication after the baseline RF transport is stable.
- Add explicit sequence and retry metadata after the baseline RF transport is stable.
- Revisit duplicate suppression once sequence metadata exists.
- Consider bundled multi-field telemetry only if real airtime testing shows one-message-per-packet is too expensive.
- If payloads ever grow beyond `255` bytes, introduce a later RF envelope version with a larger payload-length field.

## Possible Next RF Improvements

These are not committed roadmap items, but they are reasonable candidates once the current baseline has had more bench time.

- make RF diagnostics more structured if deeper field testing needs them
- add clearer dashboard comms-status indicators derived from ground and satellite heartbeat freshness
- revisit ground-station queueing if the host ever needs more than one outstanding satellite-bound command
- revisit broadcast behaviour if more RF nodes are added later

## Historical Summary

The completed phase-one transition path was:

1. move the satellite baseline from `MKR WiFi 1010` to `MKR WAN 1310`
2. move the ground-station baseline from `Arduino Mega 2560` to `MKR WAN 1310`
3. preserve the host-side logical protocol at the USB boundary
4. introduce the RF envelope and LoRa transport
5. keep one logical protocol message per RF packet
6. put retry ownership on the ground station
7. add busy-aware routine telemetry deferral on the satellite

That baseline is now implemented and described in [RF.md](./RF.md).
