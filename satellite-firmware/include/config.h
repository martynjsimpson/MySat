#ifndef SATELLITE_CONFIG_H
#define SATELLITE_CONFIG_H

#include <stdint.h>

namespace Config
{
  namespace Main
  {
    constexpr unsigned long serialStartupDelayMs = 6000;
  }

  namespace Protocol
  {
    constexpr unsigned long resetAckDelayMs = 50;
  }

  namespace Telemetry
  {
    constexpr bool defaultTelemetryEnabled = true;
    constexpr bool defaultReportTelemetry = false;
    constexpr bool defaultReportRtc = false;
    constexpr bool defaultReportLed = false;
    constexpr bool defaultReportBattery = false;
    constexpr bool defaultReportGps = false;
    constexpr bool defaultReportThermal = false;
    constexpr bool defaultReportImu = false;
    constexpr unsigned long defaultIntervalSeconds = 5;
    constexpr unsigned long minIntervalSeconds = 1;
    constexpr unsigned long maxIntervalSeconds = 3600;
  }

  namespace Rtc
  {
    constexpr uint16_t initialYear = 2000;
    constexpr uint8_t initialMonth = 1;
    constexpr uint8_t initialDay = 1;
    constexpr uint8_t initialHour = 0;
    constexpr uint8_t initialMinute = 0;
    constexpr uint8_t initialSecond = 0;

    constexpr uint16_t syncThresholdYear = 2026;
    constexpr uint8_t syncThresholdMonth = 3;
    constexpr uint8_t syncThresholdDay = 27;
    constexpr uint8_t syncThresholdHour = 0;
    constexpr uint8_t syncThresholdMinute = 0;
    constexpr uint8_t syncThresholdSecond = 0;

    constexpr bool defaultAutoSyncFromGps = true;
    constexpr unsigned long driftCheckIntervalSeconds = 600;
    constexpr unsigned long minResyncIntervalSeconds = 3600;
    constexpr unsigned long driftResyncThresholdSeconds = 1;
  }

  namespace Led
  {
    constexpr bool defaultEnabled = false;
    constexpr bool defaultStateOn = false;
    constexpr const char *defaultColor = "GREEN";
  }

  namespace Gps
  {
    constexpr bool defaultEnabled = true;
    constexpr unsigned long fixTimeoutMs = 10000;
    constexpr float stationarySpeedThresholdKph = 1.0f;
    constexpr int coordinateDecimalPlaces = 5;
  }

  namespace Thermal
  {
    constexpr bool defaultEnabled = true;
    constexpr uint8_t dataPin = 7;
    constexpr unsigned long minReadIntervalMs = 2000;
    constexpr uint8_t temperatureDecimalPlaces = 1;
    constexpr uint8_t humidityDecimalPlaces = 1;
  }

  namespace Imu
  {
    constexpr bool defaultEnabled = true;
    constexpr uint8_t i2cAddress = 0x68;
    constexpr unsigned long minReadIntervalMs = 100;
    constexpr uint8_t accelerationDecimalPlaces = 3;
    constexpr uint8_t gyroDecimalPlaces = 3;
  }
}

#endif
