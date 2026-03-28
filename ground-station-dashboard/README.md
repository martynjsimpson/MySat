# Ground Station

Node-RED based ground station for MySat.

This directory is the source-controlled home of the ground-station dashboard prototype. The canonical flow file is [flows/main.json](./flows/main.json), and the local Node-RED settings file points the runtime at that flow directly.

## Requirements

- Node.js and npm
- Node-RED runtime
- `node-red-node-serialport`
- `@flowfuse/node-red-dashboard`

## Project layout

- `flows/main.json` - main Node-RED flow under source control
- `settings.js` - Node-RED project-local settings
- `package.json` - local package metadata and start scripts
- `.gitignore` - runtime artifacts that should stay out of git

## Serial settings

- Baud: 115200
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

## Dashboard and flow behavior

The current flow includes:

- serial receive and transmit
- protocol parsing
- telemetry state storage
- flattened state for UI consumption
- dashboard cards, text widgets, tables, and a charge gauge
- command injects for LED and telemetry testing

Dashboard paths from the current flow:

- Editor default: `http://localhost:1880/`
- Dashboard: `http://localhost:1880/dashboard/page1`
- World Map: `http://localhost:1880/worldmap/`

## Notes

- The flow currently stores the serial port path inside `flows/main.json`, so macOS device naming may need adjustment.
- The flow is designed around the documented `TIME,TYPE,...` serial protocol and should stay aligned with [../documentation/Protocol.md](../documentation/Protocol.md) and [../documentation/Telemetry.md](../documentation/Telemetry.md).
