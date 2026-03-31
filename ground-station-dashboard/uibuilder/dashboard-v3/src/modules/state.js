export function createState() {
  return {
    nowMs: Date.now(),
    freshnessThresholdMs: 10000,
    payload: {
      tlm: {},
      ackLog: [],
      errorLog: [],
      packetLog: [],
      lastAck: null,
      lastErr: null,
    },
    telemetryInterval: '5',
    getSelections: {
      GROUND: 'HEARTBEAT_N',
      TELEMETRY: 'TELEMETRY',
      RTC: 'CURRENT_TIME',
      BATTERY: 'STATE',
      GPS: 'LATITUDE_D',
      THERMAL: 'TEMPERATURE_C',
      IMU: 'HEADING_DEG',
      ADCS: 'HEADING_DEG',
    },
  }
}

export const stateActions = {
  setPayload(state, payload) {
    state.payload = payload
    state.freshnessThresholdMs = payload.freshnessThresholdMs || 10000
  },
}
