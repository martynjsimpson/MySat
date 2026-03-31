#ifndef GROUND_STATION_COMMANDS_H
#define GROUND_STATION_COMMANDS_H

#include <stdint.h>

#include "sender.h"

struct GroundCommandContext
{
  uint32_t currentEpochSeconds = 0;
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

  uint32_t *clockBaseEpochSeconds = nullptr;
  unsigned long *clockBaseMillis = nullptr;
  bool *clockSyncedState = nullptr;
  GroundClockSource *clockSourceState = nullptr;
  bool *telemetryEnabledState = nullptr;

  void (*performReset)() = nullptr;
};

bool handleGroundCommandLine(const char *line, GroundCommandContext &context);

#endif
