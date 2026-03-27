#include "sender.h"

#include <Arduino.h>

#include "rtc.h"

namespace
{
  void printTwoDigits(unsigned long value)
  {
    if (value < 10)
    {
      Serial.print('0');
    }
    Serial.print(value);
  }

  void printTimestampHms()
  {
    const unsigned long totalSeconds = getTimestamp();
    const unsigned long hours = totalSeconds / 3600;
    const unsigned long minutes = (totalSeconds % 3600) / 60;
    const unsigned long seconds = totalSeconds % 60;

    printTwoDigits(hours);
    Serial.print(':');
    printTwoDigits(minutes);
    Serial.print(':');
    printTwoDigits(seconds);
  }
} // namespace

void sendAck(const char *target, const char *value)
{
  printTimestampHms();
  Serial.print(",ACK,");
  Serial.print(target);
  Serial.print(",");
  Serial.println(value);
}

void sendError(const char *errorCode)
{
  printTimestampHms();
  Serial.print(",ERR,");
  Serial.println(errorCode);
}

void sendTelemetry(const char *target, const char *parameter, const char *value)
{
  printTimestampHms();
  Serial.print(",TLM,");
  Serial.print(target);
  Serial.print(",");
  Serial.print(parameter);
  Serial.print(",");
  Serial.println(value);
}

void sendTelemetryULong(const char *target, const char *parameter, unsigned long value)
{
  printTimestampHms();
  Serial.print(",TLM,");
  Serial.print(target);
  Serial.print(",");
  Serial.print(parameter);
  Serial.print(",");
  Serial.println(value);
}

void sendTelemetryFloat(const char *target, const char *parameter, float value, int decimals)
{
  printTimestampHms();
  Serial.print(",TLM,");
  Serial.print(target);
  Serial.print(",");
  Serial.print(parameter);
  Serial.print(",");
  Serial.println(value, decimals);
}
