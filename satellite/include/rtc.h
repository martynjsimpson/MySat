#ifndef RTC_HELPER_H
#define RTC_HELPER_H

#include <Arduino.h>

#include "commands.h"

void setupRtc();
unsigned long getUptimeSeconds();
bool getCurrentTimestampIso(char *buffer, size_t bufferSize);
bool setCurrentTimeIso(const char *isoTimestamp);
bool setCurrentTimeUnix(unsigned long epochSeconds);
bool isClockSynchronized();
void reportRtcTelemetryStatus();
void reportRtcCurrentTime();
void reportRtcSyncStatus();
void reportRtcStatus();
void handleGetRtc(const Command &cmd);
void handleSetRtc(const Command &cmd);

#endif
