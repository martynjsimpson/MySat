#ifndef GROUND_STATION_SENDER_H
#define GROUND_STATION_SENDER_H

#include <stdint.h>

#include "clock.h"

struct GroundStatusSnapshot
{
  unsigned long heartbeatCount = 0;
  bool telemetryEnabled = false;
  ClockSource clockSource = CLOCK_SOURCE_UNSYNC;
  bool radioReady = false;
  bool pending = false;
  bool clockSynced = false;
  unsigned long txPacketCount = 0;
  unsigned long rxPacketCount = 0;
  unsigned long dropPacketCount = 0;
  unsigned long lastRetryAttempt = 0;
  const char *lastDropReason = "NONE";
};

void sendAck(uint32_t epochSeconds, const char *value);
void sendError(uint32_t epochSeconds, const char *errorCode, const char *context = nullptr);
void sendTelemetry(uint32_t epochSeconds, const char *parameter, const char *value);
void sendTelemetryULong(uint32_t epochSeconds, const char *parameter, unsigned long value);
void sendStatusSnapshot(uint32_t epochSeconds, const GroundStatusSnapshot &snapshot);

#endif
