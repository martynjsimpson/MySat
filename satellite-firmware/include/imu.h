#ifndef IMU_HELPER_H
#define IMU_HELPER_H

#include "commands.h"

struct ImuReadings
{
  bool enabled;
  bool available;
  float xMs2;
  float yMs2;
  float zMs2;
  float gyroXDps;
  float gyroYDps;
  float gyroZDps;
  float magXUt;
  float magYUt;
  float magZUt;
  float headingDeg;
};

void setupImu();
void updateImu();
void handleGetImu(const Command &cmd);
void handleSetImu(const Command &cmd);
void reportImuStatus();
ImuReadings getImuReadings();

#endif
