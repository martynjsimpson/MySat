#include "telemetry.h"

#include "gps.h"
#include "led.h"
#include "pmic.h"
#include "rtc.h"
#include "sender.h"
#include "status.h"

namespace
{
  bool telemetryEnabled;
  bool telemetryTelemetryEnabled;
  bool rtcTelemetryEnabled;
  bool ledTelemetryEnabled;
  bool batteryTelemetryEnabled;
  bool gpsTelemetryEnabled;
  unsigned long telemetryIntervalSeconds;
  unsigned long lastTelemetryTime = 0;

  bool *getTargetTelemetryFlag(TargetType target)
  {
    switch (target)
    {
    case TARGET_TELEMETRY:
      return &telemetryTelemetryEnabled;
    case TARGET_RTC:
      return &rtcTelemetryEnabled;
    case TARGET_LED:
      return &ledTelemetryEnabled;
    case TARGET_BATTERY:
      return &batteryTelemetryEnabled;
    case TARGET_GPS:
      return &gpsTelemetryEnabled;
    default:
      return nullptr;
    }
  }

  const char *targetToToken(TargetType target)
  {
    switch (target)
    {
    case TARGET_TELEMETRY:
      return "TELEMETRY";
    case TARGET_RTC:
      return "RTC";
    case TARGET_LED:
      return "LED";
    case TARGET_BATTERY:
      return "BATTERY";
    case TARGET_GPS:
      return "GPS";
    default:
      return nullptr;
    }
  }

}

void setupTelemetry()
{
  telemetryEnabled = true;
  telemetryTelemetryEnabled = false;
  rtcTelemetryEnabled = false;
  ledTelemetryEnabled = false;
  batteryTelemetryEnabled = false;
  gpsTelemetryEnabled = false;
  telemetryIntervalSeconds = 5;
  lastTelemetryTime = getUptimeSeconds();
}

bool isTargetTelemetryEnabled(TargetType target)
{
  bool *flag = getTargetTelemetryFlag(target);
  if (flag == nullptr)
  {
    return false;
  }

  return *flag;
}

void handleSetTelemetry(const Command &cmd)
{
  switch (cmd.parameter)
  {
  case PARAM_ENABLE:
    if (cmd.value == VALUE_TRUE)
    {
      telemetryEnabled = true;
      sendAck("TELEMETRY", "ENABLE");
    }
    else if (cmd.value == VALUE_FALSE)
    {
      telemetryEnabled = false;
      sendAck("TELEMETRY", "DISABLE");
    }
    else
    {
      sendError("BAD_VALUE");
    }
    break;

  case PARAM_INTERVAL_S:
    if (!cmd.hasNumericValue)
    {
      sendError("BAD_VALUE");
      return;
    }

    if (cmd.numericValue < 1 || cmd.numericValue > 3600)
    {
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

void handleSetTargetTelemetry(const Command &cmd)
{
  bool *flag = getTargetTelemetryFlag(cmd.target);
  const char *targetToken = targetToToken(cmd.target);

  if (flag == nullptr || targetToken == nullptr)
  {
    sendError("UNKNOWN_TARGET");
    return;
  }

  if (cmd.value == VALUE_ENABLE || cmd.value == VALUE_TRUE)
  {
    *flag = true;
    sendAck(targetToken, "TELEMETRY_ENABLE");
  }
  else if (cmd.value == VALUE_DISABLE || cmd.value == VALUE_FALSE)
  {
    *flag = false;
    sendAck(targetToken, "TELEMETRY_DISABLE");
  }
  else
  {
    sendError("BAD_VALUE");
  }
}

void reportTelemetryStatus()
{
  sendTelemetry("TELEMETRY", "TELEMETRY", telemetryTelemetryEnabled ? "TRUE" : "FALSE");
  sendTelemetry("TELEMETRY", "ENABLE", telemetryEnabled ? "TRUE" : "FALSE");
  sendTelemetryULong("TELEMETRY", "INTERVAL_S", telemetryIntervalSeconds);
}

void sendTelemetrySnapshot()
{
  reportStatusHeartbeat(true);
  if (rtcTelemetryEnabled)
  {
    reportRtcStatus();
  }
  if (ledTelemetryEnabled)
  {
    reportLedStatus();
  }
  if (telemetryTelemetryEnabled)
  {
    reportTelemetryStatus();
  }
  if (batteryTelemetryEnabled)
  {
    reportBatteryStatus();
  }
  if (gpsTelemetryEnabled)
  {
    reportGpsStatus();
  }
}

void handlePeriodicTelemetry()
{
  if (!telemetryEnabled)
  {
    return;
  }

  const unsigned long now = getUptimeSeconds();

  if ((now - lastTelemetryTime) >= telemetryIntervalSeconds)
  {
    lastTelemetryTime = now;
    sendTelemetrySnapshot();
  }
}
