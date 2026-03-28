#ifndef PROTOCOL_INTERNAL_H
#define PROTOCOL_INTERNAL_H

#include "commands.h"

void executeCommand(const Command &cmd);
void processCommand(char *line);

#endif
