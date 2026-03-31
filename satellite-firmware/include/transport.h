#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <stddef.h>
#include <stdint.h>

void setupTransport();
int transportAvailable();
int transportRead();
void transportWrite(const char *text);
void transportWrite(char value);
void transportWrite(unsigned long value);
void transportWrite(float value, int decimals);
void transportSetPacketTimestamp(uint32_t timestampSeconds);
void transportWriteLine();
void transportFlush();
bool transportShouldDeferTelemetry();

#endif
