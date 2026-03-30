#ifndef ADCS_HELPER_H
#define ADCS_HELPER_H

#include "commands.h"

void setupAdcs();
void updateAdcs();
void handleGetAdcs(const Command &cmd);
void handleSetAdcs(const Command &cmd);
void reportAdcsStatus();

#endif
