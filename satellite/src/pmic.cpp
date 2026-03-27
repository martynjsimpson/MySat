#include "pmic.h"

#include <Arduino.h>
#include <BQ24195.h>

#include "sender.h"
#include "telemetry.h"

namespace {
float rawADC = 0.0f;
float voltADC = 0.0f;
float voltBatt = 0.0f;
int battPerc = 0;

const int R1 = 330000;   // resistor between battery terminal and SAMD pin PB09
const int R2 = 1000000;  // resistor between SAMD pin PB09 and ground

int maxSourceVoltage = 0;

const float batteryFullVoltage = 4.2f;
const float batteryEmptyVoltage = 2.75f;
const float batteryCapacity = 4.0f;

void updateBatteryValues() {
  rawADC = analogRead(ADC_BATTERY);
  voltADC = rawADC * (3.3f / 4095.0f);
  voltBatt = voltADC * (static_cast<float>(maxSourceVoltage) / 3.3f);

  battPerc = static_cast<int>(
      (voltBatt - batteryEmptyVoltage) * 100.0f / (batteryFullVoltage - batteryEmptyVoltage));
}
}

void setupBattery() {
  analogReference(AR_DEFAULT);
  analogReadResolution(12);

  PMIC.begin();
  PMIC.setMinimumSystemVoltage(batteryEmptyVoltage);
  PMIC.setChargeVoltage(batteryFullVoltage);
  PMIC.setChargeCurrent(0.5f);
  PMIC.enableCharge();

  maxSourceVoltage = static_cast<int>((3.3f * (R1 + R2)) / static_cast<float>(R2));
  (void)batteryCapacity;  // reserved for future tuning
}

void reportBatteryStatus() {
  updateBatteryValues();

  sendTelemetry("BATTERY", "TELEMETRY", isTargetTelemetryEnabled(TARGET_BATTERY) ? "TRUE" : "FALSE");
  sendTelemetry("BATTERY", "AVAILABLE", PMIC.isBattConnected() ? "TRUE" : "FALSE");
  sendTelemetry("BATTERY", "ON_BATTERY", PMIC.canRunOnBattery() ? "TRUE" : "FALSE");
  sendTelemetryFloat("BATTERY", "CHARGE_CURRENT_A", PMIC.getChargeCurrent(), 3);
  sendTelemetryFloat("BATTERY", "CHARGE_VOLTAGE_V", PMIC.getChargeVoltage(), 3);
  sendTelemetryULong("BATTERY", "CHARGE_PERCENT_P", static_cast<unsigned long>(battPerc));
}
