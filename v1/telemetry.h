#ifndef TELEMETRY_HELPER_H
#define TELEMETRY_HELPER_H

#include "commands.h"

void setupTelemetry();
void handleSetTelemetry(const Command &cmd);
void reportTelemetryStatus();
void sendTelemetrySnapshot();
void handlePeriodicTelemetry();

#endif
