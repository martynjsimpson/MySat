# Ground Station Dashboard

Node-RED based operator dashboard for MySat.

This directory now contains two dashboard approaches:

- the current FlowFuse Dashboard-based console in [flows/main.json](./flows/main.json)
- the new `uibuilder`-based bespoke frontend for dashboard v3 in [uibuilder/dashboard-v3/src](./uibuilder/dashboard-v3/src)

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

- the existing FlowFuse dashboard console
- the `uibuilder` node at `/dashboard-v3`

The `uibuilder` node also routes browser-originated commands back to the existing serial transmit path, so dashboard v3 uses the same backend command pipeline as the current dashboard.

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

The v3 client currently implements:

- mission-control style operations panel
- `ACK`, `ERR`, and all-packet logs
- one-row-per-system status table
- compact per-system controls that send the same command protocol as the current dashboard
- freshness-aware parameter coloring

## Notes

- The existing FlowFuse dashboard remains in the flow as a fallback while dashboard v3 is being developed.
- The previous README sections describing the old large flattened field model and widget-by-widget dashboard composition are no longer current after the move toward the v3 uibuilder architecture.
