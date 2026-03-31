#include "mode.h"

#include "config.h"
#include "sender.h"
#include "telemetry.h"
#include "telemetry_config_internal.h"

namespace
{
  MissionMode currentMode = MODE_ORBIT;

  const char *modeToToken(MissionMode mode)
  {
    switch (mode)
    {
    case MODE_LAUNCH:
      return "LAUNCH";
    case MODE_LOW_POWER:
      return "LOW_POWER";
    case MODE_ORBIT:
    default:
      return "ORBIT";
    }
  }

  bool commandValueToMode(const Command &cmd, MissionMode &outMode)
  {
    switch (cmd.value)
    {
    case VALUE_LAUNCH:
      outMode = MODE_LAUNCH;
      return true;
    case VALUE_ORBIT:
      outMode = MODE_ORBIT;
      return true;
    case VALUE_LOW_POWER:
      outMode = MODE_LOW_POWER;
      return true;
    default:
      return false;
    }
  }

  void applyModeTelemetryDefaults(MissionMode mode)
  {
    switch (mode)
    {
    case MODE_LAUNCH:
      setTargetTelemetryEnabledInternal(TARGET_MODE, Config::Mode::Launch::reportMode);
      setTargetTelemetryEnabledInternal(TARGET_TELEMETRY, Config::Mode::Launch::reportTelemetry);
      setTargetTelemetryEnabledInternal(TARGET_RTC, Config::Mode::Launch::reportRtc);
      setTargetTelemetryEnabledInternal(TARGET_BATTERY, Config::Mode::Launch::reportBattery);
      setTargetTelemetryEnabledInternal(TARGET_GPS, Config::Mode::Launch::reportGps);
      setTargetTelemetryEnabledInternal(TARGET_THERMAL, Config::Mode::Launch::reportThermal);
      setTargetTelemetryEnabledInternal(TARGET_IMU, Config::Mode::Launch::reportImu);
      setTargetTelemetryEnabledInternal(TARGET_ADCS, Config::Mode::Launch::reportAdcs);
      break;

    case MODE_LOW_POWER:
      setTargetTelemetryEnabledInternal(TARGET_MODE, Config::Mode::LowPower::reportMode);
      setTargetTelemetryEnabledInternal(TARGET_TELEMETRY, Config::Mode::LowPower::reportTelemetry);
      setTargetTelemetryEnabledInternal(TARGET_RTC, Config::Mode::LowPower::reportRtc);
      setTargetTelemetryEnabledInternal(TARGET_BATTERY, Config::Mode::LowPower::reportBattery);
      setTargetTelemetryEnabledInternal(TARGET_GPS, Config::Mode::LowPower::reportGps);
      setTargetTelemetryEnabledInternal(TARGET_THERMAL, Config::Mode::LowPower::reportThermal);
      setTargetTelemetryEnabledInternal(TARGET_IMU, Config::Mode::LowPower::reportImu);
      setTargetTelemetryEnabledInternal(TARGET_ADCS, Config::Mode::LowPower::reportAdcs);
      break;

    case MODE_ORBIT:
    default:
      setTargetTelemetryEnabledInternal(TARGET_MODE, Config::Mode::Orbit::reportMode);
      setTargetTelemetryEnabledInternal(TARGET_TELEMETRY, Config::Mode::Orbit::reportTelemetry);
      setTargetTelemetryEnabledInternal(TARGET_RTC, Config::Mode::Orbit::reportRtc);
      setTargetTelemetryEnabledInternal(TARGET_BATTERY, Config::Mode::Orbit::reportBattery);
      setTargetTelemetryEnabledInternal(TARGET_GPS, Config::Mode::Orbit::reportGps);
      setTargetTelemetryEnabledInternal(TARGET_THERMAL, Config::Mode::Orbit::reportThermal);
      setTargetTelemetryEnabledInternal(TARGET_IMU, Config::Mode::Orbit::reportImu);
      setTargetTelemetryEnabledInternal(TARGET_ADCS, Config::Mode::Orbit::reportAdcs);
      break;
    }
  }
}

void setupMode()
{
  currentMode = Config::Mode::defaultMissionMode;
  applyModeTelemetryDefaults(currentMode);
}

void reportModeStatus()
{
  sendTelemetry("MODE", "TELEMETRY", isTargetTelemetryEnabled(TARGET_MODE) ? "TRUE" : "FALSE");
  sendTelemetry("MODE", "STATE", modeToToken(currentMode));
}

void handleGetMode(const Command &cmd)
{
  if (cmd.value != VALUE_NONE)
  {
    sendError("BAD_FORMAT");
    return;
  }

  switch (cmd.parameter)
  {
  case PARAM_NONE:
  case PARAM_STATE:
    reportModeStatus();
    break;

  case PARAM_TELEMETRY:
    sendTelemetry("MODE", "TELEMETRY", isTargetTelemetryEnabled(TARGET_MODE) ? "TRUE" : "FALSE");
    break;

  default:
    sendError("BAD_PARAMETER");
    break;
  }
}

void handleSetMode(const Command &cmd)
{
  if (cmd.parameter != PARAM_STATE)
  {
    sendError("BAD_PARAMETER");
    return;
  }

  MissionMode requestedMode = currentMode;
  if (!commandValueToMode(cmd, requestedMode))
  {
    sendError("BAD_VALUE");
    return;
  }

  currentMode = requestedMode;
  applyModeTelemetryDefaults(currentMode);
  sendAck("MODE", modeToToken(currentMode));
  reportModeStatus();
}

MissionMode getMissionMode()
{
  return currentMode;
}
