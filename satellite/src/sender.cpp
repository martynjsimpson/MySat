#include "sender.h"

#include <Arduino.h>

#include "rtc.h"

namespace
{
  const char *errorContext = nullptr;

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

  bool isUnreservedContextChar(char c)
  {
    return (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') ||
           c == '-' ||
           c == '_' ||
           c == '.' ||
           c == '~';
  }

  void printHexDigit(uint8_t nibble)
  {
    if (nibble < 10)
    {
      Serial.print(static_cast<char>('0' + nibble));
    }
    else
    {
      Serial.print(static_cast<char>('A' + (nibble - 10)));
    }
  }

  void printEscapedContext(const char *context)
  {
    for (const char *p = context; *p != '\0'; ++p)
    {
      const unsigned char ch = static_cast<unsigned char>(*p);

      if (isUnreservedContextChar(static_cast<char>(ch)))
      {
        Serial.print(static_cast<char>(ch));
      }
      else
      {
        Serial.print('%');
        printHexDigit((ch >> 4) & 0x0F);
        printHexDigit(ch & 0x0F);
      }
    }
  }
} // namespace

void setErrorContext(const char *context)
{
  errorContext = context;
}

void clearErrorContext()
{
  errorContext = nullptr;
}

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
  Serial.print(errorCode);

  if (errorContext != nullptr && *errorContext != '\0')
  {
    Serial.print(",");
    printEscapedContext(errorContext);
  }

  Serial.println();
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
