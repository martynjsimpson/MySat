#ifndef SENDER_H
#define SENDER_H

void sendAck(const char *target, const char *value);
void sendError(const char *errorCode);
void sendTelemetry(const char *target, const char *parameter, const char *value);
void sendTelemetryULong(const char *target, const char *parameter, unsigned long value);
void sendTelemetryFloat(const char *target, const char *parameter, float value, int decimals = 3);

#endif
