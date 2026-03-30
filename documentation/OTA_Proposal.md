# OTA Proposal

## Purpose

This document captures early thinking about remote firmware update for the project.

It is separate from the RF transition proposal because firmware update is a different problem from normal command, telemetry, and liveness traffic.

## Project Intent

The long-term intent is for the hypothetical satellite to operate remotely, for example in another room, while still feeling closer to a real remote vehicle than a normal local-network Arduino project.

Because of that, convenience mechanisms such as local Wi-Fi maintenance do not naturally fit the spirit of the project, even if they might be technically easy.

## Discussion Summary

We considered whether the satellite firmware could be uploaded over the planned LoRa link between two `MKR WAN` boards.

The conclusion was:

- firmware OTA over LoRa is possible in principle
- but it is a poor fit for the phase-one RF design
- and it is unlikely to be a practical or pleasant update path for this project

## Why LoRa OTA Was Considered a Bad Fit

The current compiled firmware size is about `59,708` bytes.

That is not especially large for flash storage, but it is large for a low-throughput RF transfer that would need:

- chunking
- per-chunk integrity checking
- acknowledgements
- retries
- final image validation
- safe recovery if the update is interrupted

Using rough LoRa airtime estimates, a firmware image of this size could take on the order of:

- a few minutes in relatively favorable conditions
- much longer in more conservative long-range settings

That makes LoRa OTA:

- technically conceivable
- interesting as an experiment
- but a weak primary maintenance mechanism

The bigger concern is not just transfer time. It is update robustness.

A safe OTA design would need to avoid bricking the satellite if:

- power is lost mid-update
- packets are repeatedly lost
- the image is incomplete or corrupted
- the new image boots badly

That pushes the problem toward:

- bootloader support
- fail-safe update logic
- chunk tracking and resume behavior
- deliberate image validation rules

This is much larger in scope than the command-and-telemetry RF transport work.

## Conclusion

The project should not treat LoRa firmware OTA as part of the baseline RF transition.

The RF transition should stay focused on:

- command transport
- telemetry transport
- liveness / heartbeat handling

Firmware update should remain a separate concern.

## Current Direction

The current direction is:

- do not plan firmware OTA as part of the phase-one LoRa work
- keep the LoRa link focused on operational traffic
- only revisit OTA later as a separate design effort if it still feels important

## Open Questions For A Future OTA Phase

If OTA is revisited later, the questions to answer would include:

- should OTA remain intentionally out of scope in favor of deliberate manual maintenance?
- if remote update is required, what transport best matches the spirit of the project?
- is a separate maintenance channel more appropriate than the operational LoRa link?
- what bootloader or fail-safe approach would be needed to make remote update safe?
- what level of update time would actually be acceptable?
