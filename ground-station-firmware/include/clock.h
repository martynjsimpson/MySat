#ifndef GROUND_STATION_CLOCK_H
#define GROUND_STATION_CLOCK_H

#include <stddef.h>
#include <stdint.h>

enum ClockSource
{
  CLOCK_SOURCE_UNSYNC = 0,
  CLOCK_SOURCE_LOCAL,
  CLOCK_SOURCE_SATELLITE
};

constexpr uint32_t kUnsyncedEpochSeconds = 946684800UL; // 2000-01-01T00:00:00Z

void setupClock();
uint32_t currentEpochSeconds();
bool isClockSynchronized();
ClockSource getClockSource();
const char *clockSourceToken(ClockSource source);

bool setCurrentTimeIso(const char *timestamp);
bool trySyncClockFromSatelliteTimestamp(uint32_t timestampSeconds);

bool parseIsoTimestamp(const char *timestamp, uint32_t &outEpochSeconds);
void formatIsoTimestamp(uint32_t epochSeconds, char *buffer, size_t bufferSize);
bool formatPacketTimestamp(uint32_t timestampSeconds, char *timestampBuffer, size_t timestampBufferSize);

#endif
