#include "telemetry.h"

#include "led.h"
#include "pmic.h"
#include "rtc.h"
#include "sender.h"

namespace {
bool telemetryEnabled = true;
bool ledTelemetryEnabled = true;
bool batteryTelemetryEnabled = true;
unsigned long telemetryIntervalSeconds = 5;
unsigned long lastTelemetryTime = 0;

bool *getTargetTelemetryFlag(TargetType target) {
  switch (target) {
    case TARGET_LED:
      return &ledTelemetryEnabled;
    case TARGET_BATTERY:
      return &batteryTelemetryEnabled;
    default:
      return nullptr;
  }
}

const char *targetToToken(TargetType target) {
  switch (target) {
    case TARGET_LED:
      return "LED";
    case TARGET_BATTERY:
      return "BATTERY";
    default:
      return nullptr;
  }
}

void reportTargetTelemetrySetting(TargetType target) {
  const char *targetToken = targetToToken(target);
  if (targetToken == nullptr) {
    return;
  }

  sendTelemetry(targetToken, "TELEMETRY", isTargetTelemetryEnabled(target) ? "TRUE" : "FALSE");
}
}

void setupTelemetry() {
  telemetryEnabled = true;
  ledTelemetryEnabled = true;
  batteryTelemetryEnabled = true;
  telemetryIntervalSeconds = 5;
  lastTelemetryTime = getTimestamp();
}

bool isTargetTelemetryEnabled(TargetType target) {
  bool *flag = getTargetTelemetryFlag(target);
  if (flag == nullptr) {
    return false;
  }

  return *flag;
}

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

void handleSetTargetTelemetry(const Command &cmd) {
  bool *flag = getTargetTelemetryFlag(cmd.target);
  const char *targetToken = targetToToken(cmd.target);

  if (flag == nullptr || targetToken == nullptr) {
    sendError("UNKNOWN_TARGET");
    return;
  }

  if (cmd.value == VALUE_ENABLE || cmd.value == VALUE_TRUE) {
    *flag = true;
    sendAck(targetToken, "TELEMETRY_ENABLE");
    reportTargetTelemetrySetting(cmd.target);
  } else if (cmd.value == VALUE_DISABLE || cmd.value == VALUE_FALSE) {
    *flag = false;
    sendAck(targetToken, "TELEMETRY_DISABLE");
    reportTargetTelemetrySetting(cmd.target);
  } else {
    sendError("BAD_VALUE");
  }
}

void reportTelemetryStatus() {
  sendTelemetry("TELEMETRY", "ENABLE", telemetryEnabled ? "TRUE" : "FALSE");
  sendTelemetryULong("TELEMETRY", "INTERVAL_S", telemetryIntervalSeconds);
}

void sendTelemetrySnapshot() {
  if (ledTelemetryEnabled) {
    reportLedStatus();
  }
  reportTelemetryStatus();
  if (batteryTelemetryEnabled) {
    reportBatteryStatus();
  }
}

void handlePeriodicTelemetry() {
  if (!telemetryEnabled) {
    return;
  }

  const unsigned long now = getTimestamp();

  if ((now - lastTelemetryTime) >= telemetryIntervalSeconds) {
    lastTelemetryTime = now;
    sendTelemetrySnapshot();
  }
}
