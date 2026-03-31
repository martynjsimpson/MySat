#include "pmic.h"

#include <Arduino.h>
#include <BQ24195.h>

#include "config.h"
#include "sender.h"
#include "telemetry.h"

namespace
{
  bool batteryEnabled = Config::Battery::defaultEnabled;

  const float batteryFullVoltage = 4.2f;
  const float batteryEmptyVoltage = 2.75f;
  const float batteryCapacity = 3.3f;

  bool isBatteryConnected()
  {
    // Use the PMIC's dedicated battery-present status bit rather than the
    // minimum-system-voltage status bit, which can fluctuate independently.
    return PMIC.isBattConnected();
  }

  const char *batteryChargeStateToken()
  {
    switch (PMIC.chargeStatus())
    {
    case 0x00:
      return "NOT_CHARGING";
    case 0x10:
      return "PRE_CHARGE";
    case 0x20:
      return "FAST_CHARGE";
    case 0x30:
      return "CHARGE_DONE";
    default:
      return "UNKNOWN";
    }
  }

  const char *batteryHealthToken(bool batteryAvailable)
  {
    if (!batteryAvailable)
    {
      return "NONE";
    }

    if (PMIC.isBatteryInOverVoltage() || PMIC.getChargeFault() > 0)
    {
      return "FAIL";
    }

    if (PMIC.canRunOnBattery())
    {
      return "LOW_POWER";
    }

    return "OK";
  }

  void reportBatteryTelemetryStatus()
  {
    sendTelemetry("BATTERY", "TELEMETRY", isTargetTelemetryEnabled(TARGET_BATTERY) ? "TRUE" : "FALSE");
  }

  void reportBatteryEnableStatus()
  {
    sendTelemetry("BATTERY", "ENABLE", batteryEnabled ? "TRUE" : "FALSE");
  }

  void reportBatteryAvailability(bool batteryAvailable)
  {
    sendTelemetry("BATTERY", "AVAILABLE", batteryAvailable ? "TRUE" : "FALSE");
  }

  void reportBatteryState(bool batteryAvailable)
  {
    if (!batteryEnabled)
    {
      sendTelemetry("BATTERY", "STATE", "DISABLED");
      return;
    }

    if (!batteryAvailable)
    {
      sendTelemetry("BATTERY", "STATE", "ABSENT");
      return;
    }

    if (PMIC.isBatteryInOverVoltage() || PMIC.getChargeFault() > 0)
    {
      sendTelemetry("BATTERY", "STATE", "FAULT");
      return;
    }

    sendTelemetry("BATTERY", "STATE", batteryChargeStateToken());
  }

  void reportBatteryHealth(bool batteryAvailable)
  {
    sendTelemetry("BATTERY", "HEALTH", batteryHealthToken(batteryAvailable));
  }

  void reportBatteryChargeCurrent(bool batteryAvailable)
  {
    sendTelemetryFloat("BATTERY", "CHARGE_CURRENT_A", batteryAvailable ? PMIC.getChargeCurrent() : 0.0f, 3);
  }

  void reportBatteryChargeVoltage(bool batteryAvailable)
  {
    sendTelemetryFloat("BATTERY", "CHARGE_VOLTAGE_V", batteryAvailable ? PMIC.getChargeVoltage() : 0.0f, 3);
  }
}

void setupBattery()
{
  analogReference(AR_DEFAULT);
  analogReadResolution(12);

  PMIC.begin();
  PMIC.setMinimumSystemVoltage(batteryEmptyVoltage);
  PMIC.setChargeVoltage(batteryFullVoltage);
  PMIC.setChargeCurrent(batteryCapacity / 2);
  batteryEnabled = Config::Battery::defaultEnabled;
  if (batteryEnabled)
  {
    PMIC.enableCharge();
  }
  else
  {
    PMIC.disableCharge();
  }

  (void)batteryCapacity; // reserved for future tuning
  (void)batteryFullVoltage;
  (void)batteryEmptyVoltage;
}

void reportBatteryStatus()
{
  const bool batteryAvailable = batteryEnabled && isBatteryConnected();

  reportBatteryTelemetryStatus();
  reportBatteryEnableStatus();
  reportBatteryAvailability(batteryAvailable);
  reportBatteryState(batteryAvailable);
  reportBatteryHealth(batteryAvailable);
  reportBatteryChargeCurrent(batteryAvailable);
  reportBatteryChargeVoltage(batteryAvailable);
}

void handleGetBattery(const Command &cmd)
{
  if (cmd.value != VALUE_NONE)
  {
    sendError("BAD_FORMAT");
    return;
  }

  const bool batteryAvailable = batteryEnabled && isBatteryConnected();

  switch (cmd.parameter)
  {
  case PARAM_NONE:
    reportBatteryStatus();
    break;

  case PARAM_TELEMETRY:
    reportBatteryTelemetryStatus();
    break;

  case PARAM_ENABLE:
    reportBatteryEnableStatus();
    break;

  case PARAM_AVAILABLE:
    reportBatteryAvailability(batteryAvailable);
    break;

  case PARAM_STATE:
    reportBatteryState(batteryAvailable);
    break;

  case PARAM_HEALTH:
    reportBatteryHealth(batteryAvailable);
    break;

  case PARAM_CHARGE_CURRENT_A:
    reportBatteryChargeCurrent(batteryAvailable);
    break;

  case PARAM_CHARGE_VOLTAGE_V:
    reportBatteryChargeVoltage(batteryAvailable);
    break;

  default:
    sendError("BAD_PARAMETER");
    break;
  }
}

void handleSetBattery(const Command &cmd)
{
  switch (cmd.parameter)
  {
  case PARAM_ENABLE:
    if (cmd.value == VALUE_TRUE)
    {
      batteryEnabled = true;
      PMIC.enableCharge();
      sendAck("BATTERY", "ENABLE");
    }
    else if (cmd.value == VALUE_FALSE)
    {
      batteryEnabled = false;
      PMIC.disableCharge();
      sendAck("BATTERY", "DISABLE");
    }
    else
    {
      sendError("BAD_VALUE");
    }
    break;

  default:
    sendError("BAD_PARAMETER");
    break;
  }
}
