#ifndef PMIC_HELPER_H
#define PMIC_HELPER_H

#include "commands.h"

void setupBattery();
void handleGetBattery(const Command &cmd);
void handleSetBattery(const Command &cmd);
void reportBatteryStatus();

#endif
