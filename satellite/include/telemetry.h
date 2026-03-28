#ifndef TELEMETRY_HELPER_H
#define TELEMETRY_HELPER_H

#include "commands.h"

void setupTelemetry();
void handleGetTelemetry(const Command &cmd);
void handleSetTelemetry(const Command &cmd);
void handleSetTargetTelemetry(const Command &cmd);
bool isTargetTelemetryEnabled(TargetType target);
void reportTelemetryStatus();
void sendTelemetrySnapshot();
void handlePeriodicTelemetry();

#endif
