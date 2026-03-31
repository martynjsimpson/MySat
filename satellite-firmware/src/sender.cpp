#include "sender.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "rf_envelope.h"
#include "rtc.h"
#include "transport.h"

namespace
{
  constexpr unsigned long kFallbackEpochSeconds = 946684800UL; // 2000-01-01T00:00:00Z

  const char *errorContext = nullptr;
  bool telemetryBatchActive = false;
  char telemetryBatchBuffer[RfEnvelope::maxPayloadLength + 1]{};
  size_t telemetryBatchLength = 0;

  unsigned long currentPacketTimestampSeconds()
  {
    unsigned long epochSeconds = 0;
    if (!getCurrentTimeUnix(epochSeconds))
    {
      return kFallbackEpochSeconds;
    }

    return epochSeconds;
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

  void resetTelemetryBatch()
  {
    telemetryBatchLength = 0;
    telemetryBatchBuffer[0] = '\0';
  }

  void flushTelemetryBatch()
  {
    if (telemetryBatchLength == 0)
    {
      return;
    }

    transportWrite(telemetryBatchBuffer);
    transportSetPacketTimestamp(currentPacketTimestampSeconds());
    transportWriteLine();
    resetTelemetryBatch();
  }

  bool appendTelemetryLineToBatch(const char *line)
  {
    if (line == nullptr || *line == '\0')
    {
      return true;
    }

    const size_t lineLength = strlen(line);
    if (lineLength > RfEnvelope::maxPayloadLength)
    {
      return false;
    }

    const bool hasLinesAlready = telemetryBatchLength > 0;
    const size_t separatorLength = hasLinesAlready ? 1 : 0;
    const size_t requiredLength = telemetryBatchLength + separatorLength + lineLength;
    if (requiredLength > RfEnvelope::maxPayloadLength)
    {
      flushTelemetryBatch();
    }

    if (telemetryBatchLength > 0)
    {
      telemetryBatchBuffer[telemetryBatchLength++] = '\n';
    }

    memcpy(&telemetryBatchBuffer[telemetryBatchLength], line, lineLength);
    telemetryBatchLength += lineLength;
    telemetryBatchBuffer[telemetryBatchLength] = '\0';
    return true;
  }

  void emitTelemetryLine(const char *target, const char *parameter, const char *value)
  {
    char lineBuffer[RfEnvelope::maxPayloadLength + 1];
    const int written = snprintf(lineBuffer,
                                 sizeof(lineBuffer),
                                 "TLM,%s,%s,%s",
                                 target,
                                 parameter,
                                 value);
    if (written <= 0 || static_cast<size_t>(written) >= sizeof(lineBuffer))
    {
      return;
    }

    if (telemetryBatchActive)
    {
      appendTelemetryLineToBatch(lineBuffer);
      return;
    }

    transportSetPacketTimestamp(currentPacketTimestampSeconds());
    transportWrite(lineBuffer);
    transportWriteLine();
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

void beginTelemetryBatch()
{
  telemetryBatchActive = true;
  resetTelemetryBatch();
}

void endTelemetryBatch()
{
  flushTelemetryBatch();
  telemetryBatchActive = false;
}

void sendAck(const char *target, const char *value)
{
  flushTelemetryBatch();
  transportSetPacketTimestamp(currentPacketTimestampSeconds());
  transportWrite("ACK,");
  transportWrite(target);
  transportWrite(",");
  transportWrite(value);
  transportWriteLine();
}

void sendError(const char *errorCode)
{
  flushTelemetryBatch();
  transportSetPacketTimestamp(currentPacketTimestampSeconds());
  transportWrite("ERR,");
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
  emitTelemetryLine(target, parameter, value);
}

void sendTelemetryULong(const char *target, const char *parameter, unsigned long value)
{
  char valueBuffer[16];
  ultoa(value, valueBuffer, 10);
  emitTelemetryLine(target, parameter, valueBuffer);
}

void sendTelemetryFloat(const char *target, const char *parameter, float value, int decimals)
{
  char valueBuffer[24];
  snprintf(valueBuffer, sizeof(valueBuffer), "%.*f", decimals, static_cast<double>(value));

  char *trimmed = valueBuffer;
  while (*trimmed == ' ')
  {
    ++trimmed;
  }

  emitTelemetryLine(target, parameter, trimmed);
}
