# Ground Station Dashboard

Node-RED based dashboard for MySat.

This directory is the source-controlled home of the operator dashboard. The canonical flow file is [flows/main.json](./flows/main.json), and the local Node-RED settings file points the runtime at that flow directly.

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

## Flow structure

The current dashboard flow is built around a few key stages:

- serial receive and transmit
- protocol parsing into `TLM`, `ACK`, and `ERR` message shapes
- flow-context storage of latest telemetry and recent acknowledgements/errors
- a flattened dashboard view-model for widgets
- dashboard widgets, logs, controls, and map output

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

## Flatten State

The `Flatten State` function turns the nested telemetry store into a simpler widget-facing payload.

The nested state looks like:

```js
state.tlm[target][parameter] = {
  value: "...",
  time: "2026-03-29T20:12:40Z"
}
```

The flattening step turns that into dashboard fields such as:

- `rtcSync`
- `rtcSyncLabel`
- `rtcSyncTime`
- `gpsLatitude`
- `gpsLatitudeLabel`
- `gpsLatitudeTime`

It also carries:

- `time` for the latest packet time seen by the dashboard
- `freshnessThresholdMs` for shared widget freshness timing
- `lastAck`
- `lastErr`

This means widget extractors no longer need to know where telemetry is stored internally. They can read a single field family from the flattened payload and build a display message from it.

Each field is flattened into a consistent trio:

- `<field>`
- `<field>Label`
- `<field>Time`

The dashboard now uses those flattened fields in two main ways:

- direct freshness widgets for simple single-value displays
- summary-card widgets that group related values for subsystems such as `GPS`, `LED`, `RTC`, `TELEMETRY`, `THERMAL`, `IMU`, and `ADCS`

Current `Flatten State` function:

```js
let s = msg.state || {};
let tlm = s.tlm || {};
const freshnessThresholdMs = 10000;

function getValue(target, parameter, fallback = "") {
    if (tlm[target] && tlm[target][parameter]) {
        return tlm[target][parameter].value;
    }
    return fallback;
}

function getTime(target, parameter, fallback = "") {
    if (tlm[target] && tlm[target][parameter]) {
        return tlm[target][parameter].time;
    }
    return fallback;
}

let payload = {
    time: s.time || "",
    freshnessThresholdMs,
    lastAck: s.lastAck,
    lastErr: s.lastErr
};

function addField(key, label, target, parameter) {
    payload[key] = getValue(target, parameter);
    payload[key + "Label"] = label;
    payload[key + "Time"] = getTime(target, parameter);
}

addField("rtcCurrentTime", "Current Time", "RTC", "CURRENT_TIME");
addField("rtcSync", "Clock Sync", "RTC", "SYNC");
addField("rtcTelemetry", "TLM", "RTC", "TELEMETRY");

addField("ledEnable", "Enabled", "LED", "ENABLE");
addField("ledState", "State", "LED", "STATE");
addField("ledColor", "Color", "LED", "COLOR");
addField("ledTelemetry", "TLM", "LED", "TELEMETRY");

addField("telemetryEnable", "Enable", "TELEMETRY", "ENABLE");
addField("telemetryTelemetry", "TLM", "TELEMETRY", "TELEMETRY");
addField("telemetryInterval", "Interval (s)", "TELEMETRY", "INTERVAL_S");

addField("batteryTelemetry", "Battery Telemetry", "BATTERY", "TELEMETRY");
addField("batteryAvailable", "Battery Available", "BATTERY", "AVAILABLE");
addField("chargeCurrentA", "Charge Current (A)", "BATTERY", "CHARGE_CURRENT_A");
addField("chargeVoltageV", "Charge Voltage (V)", "BATTERY", "CHARGE_VOLTAGE_V");
addField("chargePercentP", "Charge Percent (%)", "BATTERY", "CHARGE_PERCENT_P");
addField("batteryVoltage", "Battery Voltage (V)", "BATTERY", "VOLTAGE_V");

addField("statusHeartbeatValue", "Heartbeat Counter", "STATUS", "HEARTBEAT_N");

addField("gpsEnable", "Enabled", "GPS", "ENABLE");
addField("gpsAvailable", "Available", "GPS", "AVAILABLE");
addField("gpsLatitude", "Latitude", "GPS", "LATITUDE_D");
addField("gpsLongitude", "Longitude", "GPS", "LONGITUDE_D");
addField("gpsAltitude", "Altitude (m)", "GPS", "ALTITUDE_M");
addField("gpsSpeed", "Speed (km/h)", "GPS", "SPEED_KPH");
addField("gpsSatellites", "Satellites", "GPS", "SATELLITES_N");
addField("gpsTelemetry", "TLM", "GPS", "TELEMETRY");

addField("thermalEnable", "Enabled", "THERMAL", "ENABLE");
addField("thermalAvailable", "Available", "THERMAL", "AVAILABLE");
addField("thermalTemperatureC", "Temperature (°C)", "THERMAL", "TEMPERATURE_C");
addField("thermalHumidityP", "Humidity (%RH)", "THERMAL", "HUMIDITY_P");
addField("thermalTelemetry", "TLM", "THERMAL", "TELEMETRY");

addField("imuEnable", "Enabled", "IMU", "ENABLE");
addField("imuAvailable", "Available", "IMU", "AVAILABLE");
addField("imuXMs2", "X (m/s²)", "IMU", "X_MS2");
addField("imuYMs2", "Y (m/s²)", "IMU", "Y_MS2");
addField("imuZMs2", "Z (m/s²)", "IMU", "Z_MS2");
addField("imuGyroXDps", "Gyro X (dps)", "IMU", "GYRO_X_DPS");
addField("imuGyroYDps", "Gyro Y (dps)", "IMU", "GYRO_Y_DPS");
addField("imuGyroZDps", "Gyro Z (dps)", "IMU", "GYRO_Z_DPS");
addField("imuTelemetry", "TLM", "IMU", "TELEMETRY");

addField("adcsEnable", "Enabled", "ADCS", "ENABLE");
addField("adcsAvailable", "Available", "ADCS", "AVAILABLE");
addField("adcsRollDeg", "Roll (deg)", "ADCS", "ROLL_DEG");
addField("adcsPitchDeg", "Pitch (deg)", "ADCS", "PITCH_DEG");
addField("adcsYawRateDps", "Yaw Rate (dps)", "ADCS", "YAW_RATE_DPS");
addField("adcsTelemetry", "TLM", "ADCS", "TELEMETRY");

msg.payload = payload;
return msg;
```

## Freshness Widgets

The dashboard now uses a reusable freshness-aware widget pattern for telemetry text fields.

Simple freshness widgets still use a payload shaped like:

```js
msg.payload = {
    label: msg.payload.rtcSyncLabel || "",
    value: msg.payload.rtcSync || "",
    time: msg.payload.rtcSyncTime || "",
    maxAgeMs: msg.payload.freshnessThresholdMs || 10000
};
return msg;
```

Summary-card widgets use grouped arrays instead:

```js
function item(label, value, time) {
    return {
        label: label || "",
        value: value || "",
        time: time || ""
    };
}

msg.payload = {
    maxAgeMs: msg.payload.freshnessThresholdMs || 10000,
    statusItems: [
        item(msg.payload.gpsEnableLabel, msg.payload.gpsEnable, msg.payload.gpsEnableTime),
        item(msg.payload.gpsAvailableLabel, msg.payload.gpsAvailable, msg.payload.gpsAvailableTime),
        item(msg.payload.gpsSatellitesLabel, msg.payload.gpsSatellites, msg.payload.gpsSatellitesTime),
        item(msg.payload.gpsTelemetryLabel, msg.payload.gpsTelemetry, msg.payload.gpsTelemetryTime)
    ],
    metricRows: [
        [
            item(msg.payload.gpsLatitudeLabel, msg.payload.gpsLatitude, msg.payload.gpsLatitudeTime),
            item(msg.payload.gpsLongitudeLabel, msg.payload.gpsLongitude, msg.payload.gpsLongitudeTime)
        ],
        [
            item(msg.payload.gpsAltitudeLabel, msg.payload.gpsAltitude, msg.payload.gpsAltitudeTime),
            item(msg.payload.gpsSpeedLabel, msg.payload.gpsSpeed, msg.payload.gpsSpeedTime)
        ]
    ]
};
return msg;
```

The widget then colors the value based on how old the last telemetry timestamp is:

- green while fresh
- red once stale
- grey when empty

The shared CSS lives in a page-level `ui-template` called `Freshness Styles`, so individual widgets only need the template/script portion.

The dashboard now includes an `IMU` group with:

- a summary card for `ENABLE`, `AVAILABLE`, `TELEMETRY`, accelerometer axes, and gyroscope axes
- `Poll`, `Enable`, `Disable`, `TLM On`, and `TLM Off` controls
- a `GET IMU Parameter` dropdown for targeted polling

The dashboard also includes an `ADCS` group with:

- a summary card for `ENABLE`, `AVAILABLE`, `TELEMETRY`, `ROLL_DEG`, `PITCH_DEG`, and `YAW_RATE_DPS`
- `Poll`, `Enable`, `Disable`, `TLM On`, and `TLM Off` controls
- a `GET ADCS Parameter` dropdown for targeted polling

Reusable freshness widget template:

```vue
<template>
  <div class="freshness-widget">
    <div class="freshness-label">{{ displayLabel }}</div>
    <div class="freshness-value" :class="valueClass">{{ displayValue }}</div>
  </div>
</template>

<script>
export default {
  data() {
    return {
      nowMs: Date.now(),
      timerId: null
    };
  },
  computed: {
    payloadData() {
      return this.msg && this.msg.payload ? this.msg.payload : {};
    },
    displayLabel() {
      return this.payloadData.label || '';
    },
    displayValue() {
      return this.payloadData.value || '';
    },
    maxAgeMs() {
      return this.payloadData.maxAgeMs || 30000;
    },
    receivedMs() {
      const timestamp = this.payloadData.time;
      if (!timestamp) {
        return null;
      }

      const parsed = Date.parse(timestamp);
      return Number.isNaN(parsed) ? null : parsed;
    },
    isFresh() {
      if (!this.displayValue || this.receivedMs === null) {
        return false;
      }

      return (this.nowMs - this.receivedMs) <= this.maxAgeMs;
    },
    valueClass() {
      if (!this.displayValue) {
        return 'freshness-empty';
      }

      return this.isFresh ? 'freshness-fresh' : 'freshness-stale';
    }
  },
  mounted() {
    this.timerId = setInterval(() => {
      this.nowMs = Date.now();
    }, 1000);
  },
  unmounted() {
    if (this.timerId) {
      clearInterval(this.timerId);
    }
  }
};
</script>
```

Shared page-level freshness CSS:

```css
.freshness-widget {
    height: 100%;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    text-align: center;
    overflow: hidden;
    padding: 0.1rem 0;
    box-sizing: border-box;
}

.freshness-label {
    font-size: 0.85rem;
    color: #FFFFFF;
    line-height: 1.1;
    margin: 0;
}

.freshness-value {
    margin-top: 0.1rem;
    font-size: 0.95rem;
    font-weight: 600;
    line-height: 1.05;
    transition: color 0.2s ease;
}

.freshness-fresh {
    color: #2e7d32;
}

.freshness-stale {
    color: #d32f2f;
}

.freshness-empty {
    color: #717171;
}
```

## Dashboard behavior

The current flow includes:

- packet parsing and storage for `TLM`, `ACK`, and `ERR`
- packet, ACK, and ERR log widgets
- freshness-aware text widgets for key telemetry values
- grouped summary-card widgets for subsystem dashboards
- battery gauge and world map output
- controls for `STATUS`, `RTC`, `TELEMETRY`, `LED`, `BATTERY`, `GPS`, `THERMAL`, `IMU`, and `ADCS`

Dashboard paths from the current flow:

- Editor default: `http://localhost:1880/`
- Dashboard: `http://localhost:1880/dashboard/page1`
- World Map: `http://localhost:1880/worldmap/`

## Notes

- The flow currently stores the serial port path inside `flows/main.json`, so macOS device naming may need adjustment.
- The flow is designed around the documented `TIME,TYPE,...` serial protocol and should stay aligned with [../documentation/Protocol.md](../documentation/Protocol.md) and [../documentation/Telemetry.md](../documentation/Telemetry.md).
- The shared freshness threshold is currently declared once in `Flatten State` as `freshnessThresholdMs`.
