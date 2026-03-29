#include "thermal.h"

#include <Arduino.h>
#include <DHT.h>

#include "config.h"
#include "sender.h"
#include "telemetry.h"

namespace
{
  struct ThermalState
  {
    bool enabled;
    bool available;
    float temperatureC;
    float humidityP;
    unsigned long lastReadMs;
    bool hasAttemptedRead;
  };

  DHT dht(Config::Thermal::dataPin, DHT11);
  ThermalState thermalState{};

  bool canReadNow(unsigned long nowMs)
  {
    return !thermalState.hasAttemptedRead ||
           (nowMs - thermalState.lastReadMs) >= Config::Thermal::minReadIntervalMs;
  }

  void reportThermalTelemetryStatus()
  {
    sendTelemetry("THERMAL", "TELEMETRY", isTargetTelemetryEnabled(TARGET_THERMAL) ? "TRUE" : "FALSE");
  }

  void reportThermalEnableStatus()
  {
    sendTelemetry("THERMAL", "ENABLE", thermalState.enabled ? "TRUE" : "FALSE");
  }

  void reportThermalAvailability()
  {
    sendTelemetry("THERMAL", "AVAILABLE", thermalState.available ? "TRUE" : "FALSE");
  }

  void reportTemperature()
  {
    sendTelemetryFloat("THERMAL", "TEMPERATURE_C", thermalState.available ? thermalState.temperatureC : 0.0f, Config::Thermal::temperatureDecimalPlaces);
  }

  void reportHumidity()
  {
    sendTelemetryFloat("THERMAL", "HUMIDITY_P", thermalState.available ? thermalState.humidityP : 0.0f, Config::Thermal::humidityDecimalPlaces);
  }
}

void setupThermal()
{
  dht.begin();
  thermalState.enabled = Config::Thermal::defaultEnabled;
  thermalState.available = false;
  thermalState.temperatureC = 0.0f;
  thermalState.humidityP = 0.0f;
  thermalState.lastReadMs = 0;
  thermalState.hasAttemptedRead = false;
}

void updateThermal()
{
  if (!thermalState.enabled)
  {
    thermalState.available = false;
    return;
  }

  const unsigned long nowMs = millis();
  if (!canReadNow(nowMs))
  {
    return;
  }

  thermalState.lastReadMs = nowMs;
  thermalState.hasAttemptedRead = true;

  const float humidity = dht.readHumidity();
  const float temperatureC = dht.readTemperature();

  if (isnan(humidity) || isnan(temperatureC))
  {
    thermalState.available = false;
    return;
  }

  thermalState.available = true;
  thermalState.humidityP = humidity;
  thermalState.temperatureC = temperatureC;
}

void reportThermalStatus()
{
  updateThermal();

  reportThermalTelemetryStatus();
  reportThermalEnableStatus();
  reportThermalAvailability();
  reportTemperature();
  reportHumidity();
}

void handleGetThermal(const Command &cmd)
{
  if (cmd.value != VALUE_NONE)
  {
    sendError("BAD_FORMAT");
    return;
  }

  updateThermal();

  switch (cmd.parameter)
  {
  case PARAM_NONE:
    reportThermalStatus();
    break;

  case PARAM_TELEMETRY:
    reportThermalTelemetryStatus();
    break;

  case PARAM_ENABLE:
    reportThermalEnableStatus();
    break;

  case PARAM_AVAILABLE:
    reportThermalAvailability();
    break;

  case PARAM_TEMPERATURE_C:
    reportTemperature();
    break;

  case PARAM_HUMIDITY_P:
    reportHumidity();
    break;

  default:
    sendError("BAD_PARAMETER");
    break;
  }
}

void handleSetThermal(const Command &cmd)
{
  switch (cmd.parameter)
  {
  case PARAM_ENABLE:
    if (cmd.value == VALUE_TRUE)
    {
      thermalState.enabled = true;
      sendAck("THERMAL", "ENABLE");
    }
    else if (cmd.value == VALUE_FALSE)
    {
      thermalState.enabled = false;
      thermalState.available = false;
      sendAck("THERMAL", "DISABLE");
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
