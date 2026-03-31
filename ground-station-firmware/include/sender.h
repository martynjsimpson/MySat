#ifndef GROUND_STATION_SENDER_H
#define GROUND_STATION_SENDER_H

#include <stdint.h>

enum GroundClockSource
{
  GROUND_CLOCK_SOURCE_UNSYNC = 0,
  GROUND_CLOCK_SOURCE_LOCAL,
  GROUND_CLOCK_SOURCE_SATELLITE
};

struct GroundStatusSnapshot
{
  unsigned long heartbeatCount = 0;
  bool telemetryEnabled = false;
  GroundClockSource clockSource = GROUND_CLOCK_SOURCE_UNSYNC;
  bool radioReady = false;
  bool pending = false;
  bool clockSynced = false;
  unsigned long txPacketCount = 0;
  unsigned long rxPacketCount = 0;
  unsigned long dropPacketCount = 0;
  unsigned long lastRetryAttempt = 0;
};

const char *groundClockSourceToken(GroundClockSource source);
void sendGroundAck(uint32_t epochSeconds, const char *value);
void sendGroundError(uint32_t epochSeconds, const char *errorCode, const char *context = nullptr);
void sendGroundTelemetry(uint32_t epochSeconds, const char *parameter, const char *value);
void sendGroundTelemetryULong(uint32_t epochSeconds, const char *parameter, unsigned long value);
void sendGroundStatusSnapshot(uint32_t epochSeconds, const GroundStatusSnapshot &snapshot);

#endif
