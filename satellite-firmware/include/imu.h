#ifndef IMU_HELPER_H
#define IMU_HELPER_H

#include "commands.h"

void setupImu();
void updateImu();
void handleGetImu(const Command &cmd);
void handleSetImu(const Command &cmd);
void reportImuStatus();

#endif
