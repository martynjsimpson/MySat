#ifndef SATELLITE_CONFIG_H
#define SATELLITE_CONFIG_H

#include <stdint.h>

#include "mode.h"

namespace Config
{
  namespace Main
  {
    constexpr unsigned long serialStartupDelayMs = 0;
  }

  namespace Protocol
  {
    constexpr unsigned long resetAckDelayMs = 50;
  }

  namespace Transport
  {
    constexpr long loraFrequencyHz = 868000000L;
    constexpr int loraTxPowerDbm = 17;
    constexpr long loraSignalBandwidthHz = 250E3;
    constexpr int loraSpreadingFactor = 7;
    constexpr int loraCodingRateDenominator = 5;
    constexpr long loraPreambleLength = 6;
    constexpr uint8_t loraSyncWord = 0x12;
    constexpr unsigned long telemetryDeferAfterActivityMs = 250;
  }

  namespace Telemetry
  {
    constexpr bool defaultTelemetryEnabled = true;
    constexpr bool defaultReportTelemetry = false;
    constexpr bool defaultReportRtc = false;
    constexpr bool defaultReportBattery = false;
    constexpr bool defaultReportGps = false;
    constexpr bool defaultReportThermal = false;
    constexpr bool defaultReportImu = false;
    constexpr bool defaultReportAdcs = false;
    constexpr unsigned long defaultIntervalSeconds = 5;
    constexpr unsigned long minIntervalSeconds = 1;
    constexpr unsigned long maxIntervalSeconds = 3600;
  }

  namespace Mode
  {
    constexpr MissionMode defaultMissionMode = MODE_ORBIT;

    namespace Launch
    {
      constexpr bool reportMode = true;
      constexpr bool reportTelemetry = false;
      constexpr bool reportRtc = true;
      constexpr bool reportBattery = true;
      constexpr bool reportGps = false;
      constexpr bool reportThermal = false;
      constexpr bool reportImu = true;
      constexpr bool reportAdcs = true;
    }

    namespace Orbit
    {
      constexpr bool reportMode = true;
      constexpr bool reportTelemetry = false;
      constexpr bool reportRtc = true;
      constexpr bool reportBattery = true;
      constexpr bool reportGps = true;
      constexpr bool reportThermal = false;
      constexpr bool reportImu = false;
      constexpr bool reportAdcs = true;
    }

    namespace LowPower
    {
      constexpr bool reportMode = true;
      constexpr bool reportTelemetry = false;
      constexpr bool reportRtc = true;
      constexpr bool reportBattery = true;
      constexpr bool reportGps = false;
      constexpr bool reportThermal = false;
      constexpr bool reportImu = false;
      constexpr bool reportAdcs = false;
    }
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

  namespace Gps
  {
    constexpr bool defaultEnabled = true;
    constexpr unsigned long fixTimeoutMs = 10000;
    constexpr float stationarySpeedThresholdKph = 1.0f;
    constexpr int coordinateDecimalPlaces = 5;
  }

  namespace Battery
  {
    constexpr bool defaultEnabled = true;
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
    constexpr uint8_t mpuI2cAddress = 0x68;
    constexpr uint8_t magnetometerI2cAddress = 0x0D;
    constexpr unsigned long minReadIntervalMs = 100;
    constexpr uint8_t accelerationDecimalPlaces = 3;
    constexpr uint8_t gyroDecimalPlaces = 3;
    constexpr uint8_t magneticDecimalPlaces = 2;
    constexpr uint8_t headingDecimalPlaces = 1;
  }

  namespace Adcs
  {
    constexpr bool defaultEnabled = true;
    constexpr uint8_t attitudeDecimalPlaces = 2;
    constexpr uint8_t yawRateDecimalPlaces = 3;
    constexpr uint8_t headingDecimalPlaces = 1;

    // Current bench wiring appears to place the magnetometer 90 degrees
    // in-plane from the MPU-6050 body frame. These mappings align raw
    // magnetometer axes into the IMU body frame used by ADCS.
    constexpr uint8_t magBodyXSourceAxis = 1;
    constexpr uint8_t magBodyYSourceAxis = 0;
    constexpr uint8_t magBodyZSourceAxis = 2;
    constexpr int8_t magBodyXSign = 1;
    constexpr int8_t magBodyYSign = -1;
    constexpr int8_t magBodyZSign = 1;
  }
}

#endif
