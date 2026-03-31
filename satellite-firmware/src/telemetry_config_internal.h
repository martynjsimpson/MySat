#ifndef TELEMETRY_CONFIG_INTERNAL_H
#define TELEMETRY_CONFIG_INTERNAL_H

#include "commands.h"

bool isTelemetryEnabledInternal();
unsigned long getTelemetryIntervalSecondsInternal();
void setTargetTelemetryEnabledInternal(TargetType target, bool enabled);

#endif
