#ifndef GPS_HELPER_H
#define GPS_HELPER_H

#include "commands.h"

void setupGps();
void updateGps();
bool getGpsTimeUnix(unsigned long &epochSeconds);
void handleGetGps(const Command &cmd);
void handleSetGps(const Command &cmd);
void reportGpsStatus();

#endif
