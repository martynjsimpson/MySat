#ifndef MODE_H
#define MODE_H

#include "commands.h"

enum MissionMode
{
  MODE_INIT = 0,
  MODE_LAUNCH,
  MODE_ORBIT,
  MODE_LOW_POWER,
  MODE_ALL
};

void setupMode();
void handleGetMode(const Command &cmd);
void handleSetMode(const Command &cmd);
void reportModeStatus();
MissionMode getMissionMode();

#endif
