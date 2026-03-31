#ifndef GROUND_TIME_UTILS_H
#define GROUND_TIME_UTILS_H

#include <stddef.h>
#include <stdint.h>

constexpr uint32_t kUnsyncedEpochSeconds = 946684800UL; // 2000-01-01T00:00:00Z

bool parseIsoTimestamp(const char *timestamp, uint32_t &outEpochSeconds);
void formatIsoTimestamp(uint32_t epochSeconds, char *buffer, size_t bufferSize);

#endif
