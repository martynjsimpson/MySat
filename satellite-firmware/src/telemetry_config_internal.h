#ifndef TELEMETRY_CONFIG_INTERNAL_H
#define TELEMETRY_CONFIG_INTERNAL_H

#include "commands.h"

bool isTelemetryEnabledInternal();
unsigned long getTelemetryIntervalSecondsInternal();
unsigned long getSkippedTelemetryCountInternal();
void incrementSkippedTelemetryCountInternal();
void setTargetTelemetryEnabledInternal(TargetType target, bool enabled);

#endif
