#include "sender.h"

#include <Arduino.h>

#include "rtc.h"
#include "transport.h"

namespace
{
  const char *errorContext = nullptr;

  void printTimestampIso()
  {
    char timestamp[21];
    if (!getCurrentTimestampIso(timestamp, sizeof(timestamp)))
    {
      transportWrite("2000-01-01T00:00:00Z");
      return;
    }

    transportWrite(timestamp);
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
      transportWrite(static_cast<char>('0' + nibble));
    }
    else
    {
      transportWrite(static_cast<char>('A' + (nibble - 10)));
    }
  }

  void printEscapedContext(const char *context)
  {
    for (const char *p = context; *p != '\0'; ++p)
    {
      const unsigned char ch = static_cast<unsigned char>(*p);

      if (isUnreservedContextChar(static_cast<char>(ch)))
      {
        transportWrite(static_cast<char>(ch));
      }
      else
      {
        transportWrite('%');
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
  printTimestampIso();
  transportWrite(",ACK,");
  transportWrite(target);
  transportWrite(",");
  transportWrite(value);
  transportWriteLine();
}

void sendError(const char *errorCode)
{
  printTimestampIso();
  transportWrite(",ERR,");
  transportWrite(errorCode);

  if (errorContext != nullptr && *errorContext != '\0')
  {
    transportWrite(",");
    printEscapedContext(errorContext);
  }

  transportWriteLine();
}

void sendTelemetry(const char *target, const char *parameter, const char *value)
{
  printTimestampIso();
  transportWrite(",TLM,");
  transportWrite(target);
  transportWrite(",");
  transportWrite(parameter);
  transportWrite(",");
  transportWrite(value);
  transportWriteLine();
}

void sendTelemetryULong(const char *target, const char *parameter, unsigned long value)
{
  printTimestampIso();
  transportWrite(",TLM,");
  transportWrite(target);
  transportWrite(",");
  transportWrite(parameter);
  transportWrite(",");
  transportWrite(value);
  transportWriteLine();
}

void sendTelemetryFloat(const char *target, const char *parameter, float value, int decimals)
{
  printTimestampIso();
  transportWrite(",TLM,");
  transportWrite(target);
  transportWrite(",");
  transportWrite(parameter);
  transportWrite(",");
  transportWrite(value, decimals);
  transportWriteLine();
}
