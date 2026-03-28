#ifndef LED_HELPER_H
#define LED_HELPER_H

#include "commands.h"

void setupLed();
void handleGetLed(const Command &cmd);
void handleSetLed(const Command &cmd);
void reportLedStatus();

#endif
