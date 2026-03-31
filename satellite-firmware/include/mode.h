#ifndef MODE_H
#define MODE_H

#include "commands.h"

enum MissionMode
{
  MODE_LAUNCH = 0,
  MODE_ORBIT,
  MODE_LOW_POWER
};

void setupMode();
void handleGetMode(const Command &cmd);
void handleSetMode(const Command &cmd);
void reportModeStatus();
MissionMode getMissionMode();

#endif
