#ifndef TELEMETRY_CONFIG_INTERNAL_H
#define TELEMETRY_CONFIG_INTERNAL_H

#include "commands.h"

bool isTelemetryEnabledInternal();
unsigned long getTelemetryIntervalSecondsInternal();
unsigned long getSkippedTelemetryCountInternal();
const char *getLastSkippedTelemetryReasonInternal();
void incrementSkippedTelemetryCountInternal();
void setLastSkippedTelemetryReasonInternal(const char *reason);
void setTargetTelemetryEnabledInternal(TargetType target, bool enabled);

#endif
