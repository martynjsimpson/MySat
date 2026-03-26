#include "sender.h"

#include <Arduino.h>

#include "rtc.h"

void sendAck(const char *target, const char *value) {
  Serial.print(getTimestamp());
  Serial.print(",ACK,");
  Serial.print(target);
  Serial.print(",");
  Serial.println(value);
}

void sendError(const char *errorCode) {
  Serial.print(getTimestamp());
  Serial.print(",ERR,");
  Serial.println(errorCode);
}

void sendTelemetry(const char *target, const char *parameter, const char *value) {
  Serial.print(getTimestamp());
  Serial.print(",TLM,");
  Serial.print(target);
  Serial.print(",");
  Serial.print(parameter);
  Serial.print(",");
  Serial.println(value);
}

void sendTelemetryULong(const char *target, const char *parameter, unsigned long value) {
  Serial.print(getTimestamp());
  Serial.print(",TLM,");
  Serial.print(target);
  Serial.print(",");
  Serial.print(parameter);
  Serial.print(",");
  Serial.println(value);
}

void sendTelemetryFloat(const char *target, const char *parameter, float value, int decimals) {
  Serial.print(getTimestamp());
  Serial.print(",TLM,");
  Serial.print(target);
  Serial.print(",");
  Serial.print(parameter);
  Serial.print(",");
  Serial.println(value, decimals);
}
