#include "pmic.h"

#include <Arduino.h>
#include <BQ24195.h>

#include "config.h"
#include "sender.h"
#include "telemetry.h"

namespace
{
  bool batteryEnabled = Config::Battery::defaultEnabled;
  float rawADC = 0.0f;
  float voltADC = 0.0f;
  float battVolt = 0.0f;
  int battPerc = 0;

  const int R1 = 330000;  // resistor between battery terminal and SAMD pin PB09
  const int R2 = 1000000; // resistor between SAMD pin PB09 and ground

  int maxSourceVoltage = 0;

  const float batteryFullVoltage = 4.2f;
  const float batteryEmptyVoltage = 2.75f;
  const float batteryCapacity = 3.3f;

  bool isBatteryConnected()
  {
    // On this hardware/library combination, canRunOnBattery() reflects battery presence. It also needs to be inverted due to a bug
    return !PMIC.canRunOnBattery();
  }

  void updateBatteryValues()
  {
    rawADC = analogRead(ADC_BATTERY);
    voltADC = rawADC * (3.3f / 4095.0f);
    battVolt = voltADC * (static_cast<float>(maxSourceVoltage) / 3.3f);

    battPerc = static_cast<int>(
        (battVolt - batteryEmptyVoltage) * 100.0f / (batteryFullVoltage - batteryEmptyVoltage));
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

  void reportBatteryChargeCurrent(bool batteryAvailable)
  {
    sendTelemetryFloat("BATTERY", "CHARGE_CURRENT_A", batteryAvailable ? PMIC.getChargeCurrent() : 0.0f, 3);
  }

  void reportBatteryChargeVoltage(bool batteryAvailable)
  {
    sendTelemetryFloat("BATTERY", "CHARGE_VOLTAGE_V", batteryAvailable ? PMIC.getChargeVoltage() : 0.0f, 3);
  }

  void reportBatteryChargePercent(bool batteryAvailable)
  {
    sendTelemetryULong("BATTERY", "CHARGE_PERCENT_P", batteryAvailable ? static_cast<unsigned long>(battPerc) : 0);
  }

  void reportBatteryVoltage(bool batteryAvailable)
  {
    sendTelemetryFloat("BATTERY", "VOLTAGE_V", batteryAvailable ? battVolt : 0.0f);
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

  maxSourceVoltage = static_cast<int>((3.3f * (R1 + R2)) / static_cast<float>(R2));
  (void)batteryCapacity; // reserved for future tuning
}

void reportBatteryStatus()
{
  const bool batteryAvailable = batteryEnabled && isBatteryConnected();

  if (batteryAvailable)
  {
    updateBatteryValues();
  }

  reportBatteryTelemetryStatus();
  reportBatteryEnableStatus();
  reportBatteryAvailability(batteryAvailable);
  reportBatteryChargeCurrent(batteryAvailable);
  reportBatteryChargeVoltage(batteryAvailable);
  reportBatteryChargePercent(batteryAvailable);
  reportBatteryVoltage(batteryAvailable);
}

void handleGetBattery(const Command &cmd)
{
  if (cmd.value != VALUE_NONE)
  {
    sendError("BAD_FORMAT");
    return;
  }

  const bool batteryAvailable = batteryEnabled && isBatteryConnected();
  if (batteryAvailable)
  {
    updateBatteryValues();
  }

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

  case PARAM_CHARGE_CURRENT_A:
    reportBatteryChargeCurrent(batteryAvailable);
    break;

  case PARAM_CHARGE_VOLTAGE_V:
    reportBatteryChargeVoltage(batteryAvailable);
    break;

  case PARAM_CHARGE_PERCENT_P:
    reportBatteryChargePercent(batteryAvailable);
    break;

  case PARAM_VOLTAGE_V:
    reportBatteryVoltage(batteryAvailable);
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
