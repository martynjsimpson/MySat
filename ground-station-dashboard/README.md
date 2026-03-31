# Ground Station Dashboard

Node-RED based operator dashboard for MySat.

This directory now contains two dashboard approaches:

- the legacy FlowFuse Dashboard-based console in [flows/main.json](./flows/main.json)
- the current `uibuilder`-based bespoke frontend for dashboard v3 in [uibuilder/dashboard-v3/src](./uibuilder/dashboard-v3/src)

The local Node-RED settings file points the runtime at [flows/main.json](./flows/main.json) directly.

## Requirements

- Node.js and npm
- Node-RED runtime
- `node-red-node-serialport`
- `@flowfuse/node-red-dashboard`
- `node-red-contrib-uibuilder`

## Project layout

- [flows/main.json](./flows/main.json) - main Node-RED flow under source control
- [uibuilder/dashboard-v3/src/index.html](./uibuilder/dashboard-v3/src/index.html) - dashboard v3 markup
- [uibuilder/dashboard-v3/src/index.css](./uibuilder/dashboard-v3/src/index.css) - dashboard v3 styling
- [uibuilder/dashboard-v3/src/index.mjs](./uibuilder/dashboard-v3/src/index.mjs) - dashboard v3 client logic
- [settings.js](./settings.js) - project-local Node-RED settings
- [package.json](./package.json) - local package metadata and start scripts

## Architecture

Node-RED remains the backend for:

- serial receive and transmit
- protocol parsing into `TLM`, `ACK`, and `ERR`
- flow-context storage of latest telemetry and recent acknowledgements/errors
- flattening current state into a client-facing payload

Dashboard v3 moves the UI into normal source files instead of a large embedded `ui-template`.

That split is now:

- Node-RED flow: transport, parsing, state, and routing
- `uibuilder/dashboard-v3/src`: layout, rendering, styling, and user interaction

## Current flow structure

The flow currently fans the flattened telemetry state to both frontends:

- the legacy FlowFuse dashboard console
- the `uibuilder` node at `/dashboard-v3`

The `uibuilder` node also routes browser-originated commands back to the existing serial transmit path, so dashboard v3 uses the same backend command pipeline as the legacy dashboard.

There is also a `V3 Sync State` function that re-sends the latest stored telemetry snapshot to dashboard v3 when the uibuilder client emits control events such as a fresh connection.

## Serial settings

- Baud: `115200`
- Current flow serial device: `/dev/tty.usbmodem2101`

Update the serial node if your device path is different.

## Running from this folder

If `node-red` is installed globally:

```bash
npm run start
```

This starts Node-RED with:

- `ground-station-dashboard/` as the user directory
- `ground-station-dashboard/settings.js` as the settings file
- `ground-station-dashboard/flows/main.json` as the active flow file

If you want stable encrypted credentials across machines or reinstalls, set `NODE_RED_CREDENTIAL_SECRET` before starting Node-RED.

If you are only updating palette dependencies:

```bash
npm install
```

## Dashboard v3

Dashboard v3 is served by `uibuilder` at:

```text
http://localhost:1880/dashboard-v3/
```

The v3 client is a bespoke mission-control style console rather than a widget grid.

Its current structure is:

- `Operations`: heartbeat, last telemetry timestamp, last acknowledgement, last error, plus top-level `PING`, full-system `POLL`, `RESET`, and custom command send
- `Visuals`: a three-block strip containing a GPS map, high-signal key data, and an artificial horizon driven by ADCS values
- `Systems`: one compact row per target with live parameters on the left and controls on the right
- `Logs`: side-by-side `ACK`, `ERR`, and all-packet logs

The v3 interaction model is:

- all panels are collapsible
- system rows keep a consistent control grid so button columns align vertically across targets
- shared live parameters are ordered first where applicable:
  - `EN`
  - status-like fields such as `STATE`, `AVL`, or `SYNC`
  - `TLM`
  - target-specific fields afterwards
- the current systems row order is:
  - `RTC`
  - `TLM`
  - `BAT`
  - `THM`
  - `GPS`
  - `IMU`
  - `ADCS`
  - `LED`
- live values are rendered as compact instrument-style readouts, with units appended in the value text where the parameter implies one
- freshness colouring is used for live telemetry-derived values and the operations summary
- the key-data visual intentionally treats `GPS Status` as a state indicator:
  - `Fix` is shown in neutral text
  - `Lost` is shown in red

Current v3 control conventions:

- `POL` on a system row sends `GET,<TARGET>,NONE,NONE`
- top-level `POLL` sends `GET,NONE,NONE,NONE`
- `EN` / `DIS` are shown only for targets that support firmware enable control
- `RTC` intentionally does not expose `EN` / `DIS` because the RTC underpins message timestamps for the protocol

Implementation notes:

- incoming payload updates patch live system values in place instead of rebuilding the systems table
- the one-second UI timer only refreshes freshness/status styling
- this avoids open dropdowns being closed by routine UI updates while telemetry is streaming
- the GPS map uses an OpenStreetMap embed rather than an in-page tile renderer
- the GPS map ignores null or `0.00000,0.00000` fixes and holds the last valid location until a new valid fix arrives

## Notes

- The legacy FlowFuse dashboard remains in the flow as a fallback while dashboard v3 continues to evolve.
- The previous README sections describing the old large flattened field model and widget-by-widget dashboard composition are no longer current after the move toward the v3 uibuilder architecture.
