#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Arduino.h>

bool telemetryEnabled = true;
unsigned long telemetryIntervalSeconds = 5;
unsigned long lastTelemetryTime = 0;


void handleSetTelemetry(const Command &cmd) {
  switch (cmd.parameter) {
    case PARAM_ENABLE:
      if (cmd.value == VALUE_TRUE) {
        telemetryEnabled = true;
        sendAck("TELEMETRY", "ENABLE");
      } else if (cmd.value == VALUE_FALSE) {
        telemetryEnabled = false;
        sendAck("TELEMETRY", "DISABLE");
      } else {
        sendError("BAD_VALUE");
      }
      break;

    case PARAM_INTERVAL_S:
      if (!cmd.hasNumericValue) {
        sendError("BAD_VALUE");
        return;
      }

      if (cmd.numericValue < 1 || cmd.numericValue > 3600) {
        sendError("BAD_VALUE");
        return;
      }

      telemetryIntervalSeconds = cmd.numericValue;
      sendAck("TELEMETRY", "INTERVAL_S");
      break;

    default:
      sendError("BAD_PARAMETER");
      break;
  }
}

void reportTelemetryStatus() {
  sendTelemetry("TELEMETRY", "ENABLE", telemetryEnabled ? "TRUE" : "FALSE");
  sendTelemetryULong("TELEMETRY", "INTERVAL_S", telemetryIntervalSeconds);
}

void sendTelemetrySnapshot() {
  reportLedStatus();
  reportTelemetryStatus();
  reportBatteryStatus();
}

void handlePeriodicTelemetry() {
  if (!telemetryEnabled) {
    return;
  }

  unsigned long now = getTimestamp();

  if ((now - lastTelemetryTime) >= telemetryIntervalSeconds) {
    lastTelemetryTime = now;
    sendTelemetrySnapshot();
  }
}

#endif
