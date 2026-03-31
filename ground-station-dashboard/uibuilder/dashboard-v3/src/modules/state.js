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
    ledColor: 'GREEN',
    telemetryInterval: '5',
    getSelections: {
      TELEMETRY: 'TELEMETRY',
      RTC: 'CURRENT_TIME',
      LED: 'STATE',
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
