#ifndef LED_HELPER_H
#define LED_HELPER_H

#include "commands.h"

void setupLed();
void handleSetLed(const Command &cmd);
void reportLedStatus();

#endif
