#include "telemetry.h"

#include "config.h"
#include "sender.h"

namespace
{
  bool telemetryEnabled;
  bool telemetryTelemetryEnabled;
  bool rtcTelemetryEnabled;
  bool ledTelemetryEnabled;
  bool batteryTelemetryEnabled;
  bool gpsTelemetryEnabled;
  unsigned long telemetryIntervalSeconds;

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
  telemetryEnabled = Config::Telemetry::defaultTelemetryEnabled;
  telemetryTelemetryEnabled = Config::Telemetry::defaultReportTelemetry;
  rtcTelemetryEnabled = Config::Telemetry::defaultReportRtc;
  ledTelemetryEnabled = Config::Telemetry::defaultReportLed;
  batteryTelemetryEnabled = Config::Telemetry::defaultReportBattery;
  gpsTelemetryEnabled = Config::Telemetry::defaultReportGps;
  telemetryIntervalSeconds = Config::Telemetry::defaultIntervalSeconds;
}

void handleGetTelemetry(const Command &cmd)
{
  if (cmd.value != VALUE_NONE)
  {
    sendError("BAD_FORMAT");
    return;
  }

  switch (cmd.parameter)
  {
  case PARAM_NONE:
    reportTelemetryStatus();
    break;

  case PARAM_TELEMETRY:
    sendTelemetry("TELEMETRY", "TELEMETRY", telemetryTelemetryEnabled ? "TRUE" : "FALSE");
    break;

  case PARAM_ENABLE:
    sendTelemetry("TELEMETRY", "ENABLE", telemetryEnabled ? "TRUE" : "FALSE");
    break;

  case PARAM_INTERVAL_S:
    sendTelemetryULong("TELEMETRY", "INTERVAL_S", telemetryIntervalSeconds);
    break;

  default:
    sendError("BAD_PARAMETER");
    break;
  }
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

    if (cmd.numericValue < Config::Telemetry::minIntervalSeconds ||
        cmd.numericValue > Config::Telemetry::maxIntervalSeconds)
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

bool isTelemetryEnabledInternal()
{
  return telemetryEnabled;
}

unsigned long getTelemetryIntervalSecondsInternal()
{
  return telemetryIntervalSeconds;
}
