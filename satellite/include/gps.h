#ifndef GPS_HELPER_H
#define GPS_HELPER_H

#include "commands.h"

void setupGps();
void updateGps();
void handleGetGps(const Command &cmd);
void handleSetGps(const Command &cmd);
void reportGpsStatus();

#endif
