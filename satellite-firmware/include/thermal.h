#ifndef THERMAL_HELPER_H
#define THERMAL_HELPER_H

#include "commands.h"

void setupThermal();
void updateThermal();
void handleGetThermal(const Command &cmd);
void handleSetThermal(const Command &cmd);
void reportThermalStatus();

#endif
