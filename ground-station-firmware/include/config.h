#ifndef GROUND_STATION_CONFIG_H
#define GROUND_STATION_CONFIG_H

#include <stdint.h>

namespace Config
{
  namespace Serial
  {
    constexpr unsigned long baudRate = 115200;
    constexpr unsigned long heartbeatIntervalMs = 5000;
    constexpr bool logRxPayloadDiagnostics = false;
  }

  namespace Transport
  {
    constexpr long loraFrequencyHz = 868000000L;
    constexpr int loraTxPowerDbm = 17;
    constexpr long loraSignalBandwidthHz = 125E3;
    constexpr int loraSpreadingFactor = 7;
    constexpr int loraCodingRateDenominator = 5;
    constexpr long loraPreambleLength = 8;
    constexpr uint8_t loraSyncWord = 0x12;
  }

  namespace Retry
  {
    constexpr unsigned long commandRetryDelayMs = 5000;
    constexpr uint8_t maxCommandRetries = 3;
  }

  namespace Receive
  {
    constexpr unsigned long duplicateSuppressWindowMs = 250;
  }

  namespace Protocol
  {
    constexpr unsigned long resetAckDelayMs = 50;
  }

  namespace Clock
  {
    constexpr bool autoSyncFromSatellite = true;
    constexpr const char *minTrustedSatelliteTimestamp = "2026-03-27T00:00:00Z";
  }

  namespace Telemetry
  {
    constexpr bool defaultEnabled = false;
  }

  namespace Led
  {
    constexpr unsigned long activityPulseMs = 120;
  }
}

#endif
