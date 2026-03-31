#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <stddef.h>

void setupTransport();
int transportAvailable();
int transportRead();
void transportWrite(const char *text);
void transportWrite(char value);
void transportWrite(unsigned long value);
void transportWrite(float value, int decimals);
void transportWriteLine();
void transportFlush();
bool transportShouldDeferTelemetry();

#endif
