#include "sender.h"

#include <Arduino.h>
#include <stdint.h>

#include "clock.h"

namespace
{
  void writeTimestamp(uint32_t epochSeconds)
  {
    char timestamp[21];
    formatIsoTimestamp(epochSeconds, timestamp, sizeof(timestamp));
    Serial.print(timestamp);
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
    for (const char *pointer = context; *pointer != '\0'; ++pointer)
    {
      const unsigned char ch = static_cast<unsigned char>(*pointer);
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

void sendAck(uint32_t epochSeconds, const char *value)
{
  writeTimestamp(epochSeconds);
  Serial.print(F(",ACK,GROUND,"));
  Serial.println(value);
}

void sendError(uint32_t epochSeconds, const char *errorCode, const char *context)
{
  writeTimestamp(epochSeconds);
  Serial.print(F(",ERR,"));
  Serial.print(errorCode);
  if (context != nullptr && *context != '\0')
  {
    Serial.print(F(","));
    printEscapedContext(context);
  }
  Serial.println();
}

void sendTelemetry(uint32_t epochSeconds, const char *parameter, const char *value)
{
  writeTimestamp(epochSeconds);
  Serial.print(F(",TLM,GROUND,"));
  Serial.print(parameter);
  Serial.print(F(","));
  Serial.println(value);
}

void sendTelemetryULong(uint32_t epochSeconds, const char *parameter, unsigned long value)
{
  writeTimestamp(epochSeconds);
  Serial.print(F(",TLM,GROUND,"));
  Serial.print(parameter);
  Serial.print(F(","));
  Serial.println(value);
}

void sendStatusSnapshot(uint32_t epochSeconds, const GroundStatusSnapshot &snapshot)
{
  char timestamp[21];
  formatIsoTimestamp(epochSeconds, timestamp, sizeof(timestamp));

  Serial.print(timestamp);
  Serial.print(F(",TLM,GROUND,HEARTBEAT_N,"));
  Serial.println(snapshot.heartbeatCount);
  Serial.print(timestamp);
  Serial.print(F(",TLM,GROUND,TELEMETRY,"));
  Serial.println(snapshot.telemetryEnabled ? F("TRUE") : F("FALSE"));
  Serial.print(timestamp);
  Serial.print(F(",TLM,GROUND,SOURCE,"));
  Serial.println(clockSourceToken(snapshot.clockSource));
  Serial.print(timestamp);
  Serial.print(F(",TLM,GROUND,RADIO,"));
  Serial.println(snapshot.radioReady ? F("READY") : F("FAILED"));
  Serial.print(timestamp);
  Serial.print(F(",TLM,GROUND,PENDING,"));
  Serial.println(snapshot.pending ? F("TRUE") : F("FALSE"));
  Serial.print(timestamp);
  Serial.print(F(",TLM,GROUND,CLOCK_SYNC,"));
  Serial.println(snapshot.clockSynced ? F("TRUE") : F("FALSE"));
  Serial.print(timestamp);
  Serial.print(F(",TLM,GROUND,TX_PACKETS_N,"));
  Serial.println(snapshot.txPacketCount);
  Serial.print(timestamp);
  Serial.print(F(",TLM,GROUND,RX_PACKETS_N,"));
  Serial.println(snapshot.rxPacketCount);
  Serial.print(timestamp);
  Serial.print(F(",TLM,GROUND,DROP_PACKETS_N,"));
  Serial.println(snapshot.dropPacketCount);
  Serial.print(timestamp);
  Serial.print(F(",TLM,GROUND,LAST_DROP_REASON,"));
  Serial.println(snapshot.lastDropReason != nullptr ? snapshot.lastDropReason : "NONE");
  Serial.print(timestamp);
  Serial.print(F(",TLM,GROUND,LAST_RETRY_N,"));
  Serial.println(snapshot.lastRetryAttempt);
  Serial.print(timestamp);
  Serial.print(F(",TLM,GROUND,CURRENT_TIME,"));
  Serial.println(timestamp);
}
