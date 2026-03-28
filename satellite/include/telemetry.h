#ifndef TELEMETRY_HELPER_H
#define TELEMETRY_HELPER_H

#include "commands.h"

void setupTelemetry();
void handleSetTelemetry(const Command &cmd);
void handleSetTargetTelemetry(const Command &cmd);
bool isTargetTelemetryEnabled(TargetType target);
void reportTelemetryStatus();
void reportStatusHeartbeat(bool incrementHeartbeat);
void sendTelemetrySnapshot();
void handlePeriodicTelemetry();

#endif
