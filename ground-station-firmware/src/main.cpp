#include <Arduino.h>
#include <LoRa.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "rf_envelope.h"

namespace
{
  constexpr size_t kLineBufferSize = RfEnvelope::maxPayloadLength + 1;
  constexpr size_t kPacketBufferSize = RfEnvelope::maxPacketLength;
  constexpr uint32_t kUnsyncedEpochSeconds = 946684800UL; // 2000-01-01T00:00:00Z

  enum PendingResponseKind
  {
    RESPONSE_NONE = 0,
    RESPONSE_ACK_OR_ERR,
    RESPONSE_TLM_OR_ERR
  };

  struct PendingCommandState
  {
    bool active = false;
    char line[kLineBufferSize]{};
    unsigned long lastSendMs = 0;
    uint8_t retryCount = 0;
    PendingResponseKind expectedResponse = RESPONSE_NONE;
  };

  struct CommandView
  {
    const char *type = nullptr;
    const char *target = nullptr;
    const char *parameter = nullptr;
    const char *value = nullptr;
  };

  enum ClockSource
  {
    CLOCK_SOURCE_UNSYNC = 0,
    CLOCK_SOURCE_LOCAL,
    CLOCK_SOURCE_SATELLITE
  };

  char hostInputBuffer[kLineBufferSize];
  size_t hostInputLength = 0;
  unsigned long lastHostHeartbeatMs = 0;
  unsigned long ledPulseStartMs = 0;
  bool ledPulseActive = false;
  unsigned long txPacketCount = 0;
  unsigned long rxPacketCount = 0;
  unsigned long dropPacketCount = 0;
  unsigned long lastRetryAttempt = 0;
  unsigned long groundHeartbeatCount = 0;
  unsigned long lastForwardedPayloadMs = 0;
  char lastForwardedPayload[kLineBufferSize]{};

  PendingCommandState pendingCommand;
  bool radioReady = false;

  uint32_t clockBaseEpochSeconds = kUnsyncedEpochSeconds;
  unsigned long clockBaseMillis = 0;
  bool clockSynced = false;
  ClockSource clockSource = CLOCK_SOURCE_UNSYNC;
  bool groundTelemetryEnabled = Config::Telemetry::defaultEnabled;

  void applyLedOutput()
  {
    digitalWrite(LED_BUILTIN, ledPulseActive ? HIGH : LOW);
  }

  void noteLedActivity()
  {
    ledPulseActive = true;
    ledPulseStartMs = millis();
    applyLedOutput();
  }

  void updateLed()
  {
    if (!ledPulseActive)
    {
      return;
    }

    if ((millis() - ledPulseStartMs) < Config::Led::activityPulseMs)
    {
      return;
    }

    ledPulseActive = false;
    applyLedOutput();
  }

  bool isLeapYear(int year)
  {
    return ((year % 4) == 0 && (year % 100) != 0) || ((year % 400) == 0);
  }

  uint32_t daysFromCivil(int year, unsigned month, unsigned day)
  {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return static_cast<uint32_t>(era * 146097 + static_cast<int>(doe) - 719468);
  }

  void civilFromDays(int64_t z, int &year, unsigned &month, unsigned &day)
  {
    z += 719468;
    const int era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    year = static_cast<int>(yoe) + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    day = doy - (153 * mp + 2) / 5 + 1;
    month = mp + (mp < 10 ? 3 : -9);
    year += (month <= 2);
  }

  bool parseTwoDigits(const char *text, int &value)
  {
    if (text[0] < '0' || text[0] > '9' || text[1] < '0' || text[1] > '9')
    {
      return false;
    }

    value = (text[0] - '0') * 10 + (text[1] - '0');
    return true;
  }

  bool parseFourDigits(const char *text, int &value)
  {
    value = 0;
    for (size_t index = 0; index < 4; ++index)
    {
      if (text[index] < '0' || text[index] > '9')
      {
        return false;
      }

      value = (value * 10) + (text[index] - '0');
    }

    return true;
  }

  bool parseIsoTimestamp(const char *timestamp, uint32_t &outEpochSeconds)
  {
    if (timestamp == nullptr || strlen(timestamp) != 20)
    {
      return false;
    }

    if (timestamp[4] != '-' ||
        timestamp[7] != '-' ||
        timestamp[10] != 'T' ||
        timestamp[13] != ':' ||
        timestamp[16] != ':' ||
        timestamp[19] != 'Z')
    {
      return false;
    }

    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;

    if (!parseFourDigits(timestamp, year) ||
        !parseTwoDigits(timestamp + 5, month) ||
        !parseTwoDigits(timestamp + 8, day) ||
        !parseTwoDigits(timestamp + 11, hour) ||
        !parseTwoDigits(timestamp + 14, minute) ||
        !parseTwoDigits(timestamp + 17, second))
    {
      return false;
    }

    if (month < 1 || month > 12 || day < 1 || hour > 23 || minute > 59 || second > 59)
    {
      return false;
    }

    static const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int maxDay = daysInMonth[month - 1];
    if (month == 2 && isLeapYear(year))
    {
      maxDay = 29;
    }

    if (day > maxDay)
    {
      return false;
    }

    const uint32_t days = daysFromCivil(year, static_cast<unsigned>(month), static_cast<unsigned>(day));
    outEpochSeconds = (days * 86400UL) +
                      (static_cast<uint32_t>(hour) * 3600UL) +
                      (static_cast<uint32_t>(minute) * 60UL) +
                      static_cast<uint32_t>(second);
    return true;
  }

  void formatIsoTimestamp(uint32_t epochSeconds, char *buffer, size_t bufferSize)
  {
    if (buffer == nullptr || bufferSize < 21)
    {
      return;
    }

    const uint32_t days = epochSeconds / 86400UL;
    uint32_t secondsOfDay = epochSeconds % 86400UL;

    int year = 0;
    unsigned month = 0;
    unsigned day = 0;
    civilFromDays(days, year, month, day);

    const unsigned hour = secondsOfDay / 3600UL;
    secondsOfDay %= 3600UL;
    const unsigned minute = secondsOfDay / 60UL;
    const unsigned second = secondsOfDay % 60UL;

    buffer[0] = static_cast<char>('0' + ((year / 1000) % 10));
    buffer[1] = static_cast<char>('0' + ((year / 100) % 10));
    buffer[2] = static_cast<char>('0' + ((year / 10) % 10));
    buffer[3] = static_cast<char>('0' + (year % 10));
    buffer[4] = '-';
    buffer[5] = static_cast<char>('0' + ((month / 10) % 10));
    buffer[6] = static_cast<char>('0' + (month % 10));
    buffer[7] = '-';
    buffer[8] = static_cast<char>('0' + ((day / 10) % 10));
    buffer[9] = static_cast<char>('0' + (day % 10));
    buffer[10] = 'T';
    buffer[11] = static_cast<char>('0' + ((hour / 10) % 10));
    buffer[12] = static_cast<char>('0' + (hour % 10));
    buffer[13] = ':';
    buffer[14] = static_cast<char>('0' + ((minute / 10) % 10));
    buffer[15] = static_cast<char>('0' + (minute % 10));
    buffer[16] = ':';
    buffer[17] = static_cast<char>('0' + ((second / 10) % 10));
    buffer[18] = static_cast<char>('0' + (second % 10));
    buffer[19] = 'Z';
    buffer[20] = '\0';
  }

  uint32_t currentEpochSeconds()
  {
    return clockBaseEpochSeconds + ((millis() - clockBaseMillis) / 1000UL);
  }

  const __FlashStringHelper *clockSourceLabel()
  {
    switch (clockSource)
    {
    case CLOCK_SOURCE_LOCAL:
      return F("LOCAL");
    case CLOCK_SOURCE_SATELLITE:
      return F("SATELLITE");
    case CLOCK_SOURCE_UNSYNC:
    default:
      return F("UNSYNC");
    }
  }

  void writeCurrentTimestamp()
  {
    char timestamp[21];
    formatIsoTimestamp(currentEpochSeconds(), timestamp, sizeof(timestamp));
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

  void sendGroundAck(const char *value)
  {
    writeCurrentTimestamp();
    Serial.print(F(",ACK,GROUND,"));
    Serial.println(value);
  }

  void sendGroundError(const char *errorCode, const char *context = nullptr)
  {
    writeCurrentTimestamp();
    Serial.print(F(",ERR,"));
    Serial.print(errorCode);
    if (context != nullptr && *context != '\0')
    {
      Serial.print(F(","));
      printEscapedContext(context);
    }
    Serial.println();
  }

  void sendGroundTelemetry(const char *parameter, const char *value)
  {
    writeCurrentTimestamp();
    Serial.print(F(",TLM,GROUND,"));
    Serial.print(parameter);
    Serial.print(F(","));
    Serial.println(value);
  }

  void sendGroundTelemetryFlash(const char *parameter, const __FlashStringHelper *value)
  {
    writeCurrentTimestamp();
    Serial.print(F(",TLM,GROUND,"));
    Serial.print(parameter);
    Serial.print(F(","));
    Serial.println(value);
  }

  void sendGroundTelemetryULong(const char *parameter, unsigned long value)
  {
    writeCurrentTimestamp();
    Serial.print(F(",TLM,GROUND,"));
    Serial.print(parameter);
    Serial.print(F(","));
    Serial.println(value);
  }

  void sendGroundStatusSnapshot()
  {
    char timestamp[21];
    formatIsoTimestamp(currentEpochSeconds(), timestamp, sizeof(timestamp));

    Serial.print(timestamp);
    Serial.print(F(",TLM,GROUND,HEARTBEAT_N,"));
    Serial.println(groundHeartbeatCount);
    Serial.print(timestamp);
    Serial.print(F(",TLM,GROUND,TELEMETRY,"));
    Serial.println(groundTelemetryEnabled ? F("TRUE") : F("FALSE"));
    Serial.print(timestamp);
    Serial.print(F(",TLM,GROUND,SOURCE,"));
    Serial.println(clockSourceLabel());
    Serial.print(timestamp);
    Serial.print(F(",TLM,GROUND,RADIO,"));
    Serial.println(radioReady ? F("READY") : F("FAILED"));
    Serial.print(timestamp);
    Serial.print(F(",TLM,GROUND,PENDING,"));
    Serial.println(pendingCommand.active ? F("TRUE") : F("FALSE"));
    Serial.print(timestamp);
    Serial.print(F(",TLM,GROUND,CLOCK_SYNC,"));
    Serial.println(clockSynced ? F("TRUE") : F("FALSE"));
    Serial.print(timestamp);
    Serial.print(F(",TLM,GROUND,TX_PACKETS_N,"));
    Serial.println(txPacketCount);
    Serial.print(timestamp);
    Serial.print(F(",TLM,GROUND,RX_PACKETS_N,"));
    Serial.println(rxPacketCount);
    Serial.print(timestamp);
    Serial.print(F(",TLM,GROUND,DROP_PACKETS_N,"));
    Serial.println(dropPacketCount);
    Serial.print(timestamp);
    Serial.print(F(",TLM,GROUND,LAST_RETRY_N,"));
    Serial.println(lastRetryAttempt);
    Serial.print(timestamp);
    Serial.print(F(",TLM,GROUND,CURRENT_TIME,"));
    Serial.println(timestamp);
  }

  const __FlashStringHelper *decodeStatusLabel(RfEnvelope::DecodeStatus status)
  {
    switch (status)
    {
    case RfEnvelope::DECODE_PACKET_TOO_SHORT:
      return F("PACKET_TOO_SHORT");
    case RfEnvelope::DECODE_UNSUPPORTED_VERSION:
      return F("UNSUPPORTED_VERSION");
    case RfEnvelope::DECODE_LENGTH_MISMATCH:
      return F("LENGTH_MISMATCH");
    case RfEnvelope::DECODE_CRC_MISMATCH:
      return F("CRC_MISMATCH");
    case RfEnvelope::DECODE_NOT_FOR_DEVICE:
      return F("NOT_FOR_DEVICE");
    case RfEnvelope::DECODE_PAYLOAD_TOO_LARGE:
      return F("PAYLOAD_TOO_LARGE");
    case RfEnvelope::DECODE_OK:
    default:
      return F("OK");
    }
  }

  void emitDropTelemetry(RfEnvelope::DecodeStatus status)
  {
    sendGroundTelemetryFlash("LAST_ERROR", decodeStatusLabel(status));
    sendGroundTelemetryULong("DROP_PACKETS_N", dropPacketCount);
  }

  bool extractPayloadTimestamp(const char *payload, char *timestampBuffer, size_t timestampBufferSize)
  {
    if (payload == nullptr || timestampBuffer == nullptr || timestampBufferSize < 21)
    {
      return false;
    }

    const char *firstNewline = strchr(payload, '\n');
    if (firstNewline != nullptr)
    {
      const size_t length = static_cast<size_t>(firstNewline - payload);
      if (length != 20 || length >= timestampBufferSize)
      {
        return false;
      }

      memcpy(timestampBuffer, payload, length);
      timestampBuffer[length] = '\0';
      return true;
    }

    const char *firstComma = strchr(payload, ',');
    if (firstComma == nullptr)
    {
      return false;
    }

    const size_t length = static_cast<size_t>(firstComma - payload);
    if (length != 20 || length >= timestampBufferSize)
    {
      return false;
    }

    memcpy(timestampBuffer, payload, length);
    timestampBuffer[length] = '\0';
    return true;
  }

  bool isTrustedSatelliteTimestamp(const char *timestamp)
  {
    if (timestamp == nullptr)
    {
      return false;
    }

    return strcmp(timestamp, Config::Clock::minTrustedSatelliteTimestamp) >= 0;
  }

  void maybeSyncClockFromSatellitePayload(const char *payload)
  {
    if (!Config::Clock::autoSyncFromSatellite || clockSource != CLOCK_SOURCE_UNSYNC)
    {
      return;
    }

    char timestamp[21];
    if (!extractPayloadTimestamp(payload, timestamp, sizeof(timestamp)))
    {
      return;
    }

    if (!isTrustedSatelliteTimestamp(timestamp))
    {
      return;
    }

    uint32_t epochSeconds = 0;
    if (!parseIsoTimestamp(timestamp, epochSeconds))
    {
      return;
    }

    clockBaseEpochSeconds = epochSeconds;
    clockBaseMillis = millis();
    clockSynced = true;
    clockSource = CLOCK_SOURCE_SATELLITE;

    sendGroundTelemetry("CURRENT_TIME", timestamp);
    sendGroundTelemetryFlash("SOURCE", F("SATELLITE"));
    sendGroundTelemetryFlash("CLOCK_SYNC", F("TRUE"));
  }

  PendingResponseKind expectedResponseForCommand(const char *line)
  {
    if (line == nullptr)
    {
      return RESPONSE_NONE;
    }

    if (strncmp(line, "GET,", 4) == 0)
    {
      return RESPONSE_TLM_OR_ERR;
    }

    if (strncmp(line, "SET,", 4) == 0 ||
        strncmp(line, "PING,", 5) == 0 ||
        strncmp(line, "RESET,", 6) == 0)
    {
      return RESPONSE_ACK_OR_ERR;
    }

    return RESPONSE_NONE;
  }

  bool sendPayloadToSatellite(const char *payload)
  {
    if (!radioReady || payload == nullptr || *payload == '\0')
    {
      return false;
    }

    uint8_t packetBuffer[kPacketBufferSize];
    size_t packetLength = 0;
    if (!RfEnvelope::encodePacket(RfEnvelope::groundStationDeviceId,
                                  RfEnvelope::satelliteDeviceId,
                                  payload,
                                  packetBuffer,
                                  sizeof(packetBuffer),
                                  packetLength))
    {
      return false;
    }

    if (LoRa.beginPacket() != 1)
    {
      return false;
    }

    LoRa.write(packetBuffer, packetLength);
    if (LoRa.endPacket(false) != 1)
    {
      return false;
    }

    txPacketCount++;
    noteLedActivity();
    return true;
  }

  void startPendingCommand(const char *line)
  {
    if (line == nullptr)
    {
      pendingCommand.active = false;
      return;
    }

    strncpy(pendingCommand.line, line, sizeof(pendingCommand.line) - 1);
    pendingCommand.line[sizeof(pendingCommand.line) - 1] = '\0';
    pendingCommand.lastSendMs = millis();
    pendingCommand.retryCount = 0;
    pendingCommand.expectedResponse = expectedResponseForCommand(line);
    pendingCommand.active = pendingCommand.expectedResponse != RESPONSE_NONE;
  }

  void clearPendingCommand()
  {
    pendingCommand.active = false;
    pendingCommand.line[0] = '\0';
    pendingCommand.lastSendMs = 0;
    pendingCommand.retryCount = 0;
    pendingCommand.expectedResponse = RESPONSE_NONE;
  }

  bool payloadMatchesPendingResponse(const char *payload)
  {
    if (!pendingCommand.active || payload == nullptr)
    {
      return false;
    }

    const char *firstComma = strchr(payload, ',');
    if (firstComma == nullptr)
    {
      return false;
    }

    const char *typeStart = firstComma + 1;
    const char *secondComma = strchr(typeStart, ',');
    if (secondComma == nullptr)
    {
      return false;
    }

    const size_t typeLength = static_cast<size_t>(secondComma - typeStart);
    if (typeLength == 3 && strncmp(typeStart, "ERR", 3) == 0)
    {
      return true;
    }

    if (pendingCommand.expectedResponse == RESPONSE_ACK_OR_ERR)
    {
      return typeLength == 3 && strncmp(typeStart, "ACK", 3) == 0;
    }

    if (pendingCommand.expectedResponse == RESPONSE_TLM_OR_ERR)
    {
      return typeLength == 3 && strncmp(typeStart, "TLM", 3) == 0;
    }

    return false;
  }

  void forwardPayloadToHost(const char *payload)
  {
    if (payload == nullptr)
    {
      return;
    }

    const char *firstNewline = strchr(payload, '\n');
    if (firstNewline != nullptr)
    {
      const size_t timestampLength = static_cast<size_t>(firstNewline - payload);
      if (timestampLength == 20)
      {
        char timestamp[21];
        memcpy(timestamp, payload, timestampLength);
        timestamp[timestampLength] = '\0';

        const char *lineStart = firstNewline + 1;
        while (*lineStart != '\0')
        {
          const char *lineEnd = strchr(lineStart, '\n');
          if (lineEnd == nullptr)
          {
            if (*lineStart != '\0')
            {
              Serial.print(timestamp);
              Serial.print(',');
              Serial.println(lineStart);
            }
            return;
          }

          if (lineEnd > lineStart)
          {
            Serial.print(timestamp);
            Serial.print(',');
            Serial.write(lineStart, static_cast<size_t>(lineEnd - lineStart));
            Serial.println();
          }

          lineStart = lineEnd + 1;
        }
        return;
      }
    }

    const char *lineStart = payload;
    while (*lineStart != '\0')
    {
      const char *lineEnd = strchr(lineStart, '\n');
      if (lineEnd == nullptr)
      {
        Serial.println(lineStart);
        return;
      }

      if (lineEnd > lineStart)
      {
        Serial.write(lineStart, static_cast<size_t>(lineEnd - lineStart));
        Serial.println();
      }

      lineStart = lineEnd + 1;
    }
  }

  bool shouldSuppressDuplicatePayload(const char *payload)
  {
    if (payload == nullptr || *payload == '\0')
    {
      return false;
    }

    if (lastForwardedPayload[0] == '\0')
    {
      return false;
    }

    if (strcmp(payload, lastForwardedPayload) != 0)
    {
      return false;
    }

    return (millis() - lastForwardedPayloadMs) <= Config::Receive::duplicateSuppressWindowMs;
  }

  void rememberForwardedPayload(const char *payload)
  {
    if (payload == nullptr || *payload == '\0')
    {
      lastForwardedPayload[0] = '\0';
      lastForwardedPayloadMs = 0;
      return;
    }

    strncpy(lastForwardedPayload, payload, sizeof(lastForwardedPayload) - 1);
    lastForwardedPayload[sizeof(lastForwardedPayload) - 1] = '\0';
    lastForwardedPayloadMs = millis();
  }

  void emitGroundHeartbeat()
  {
    const unsigned long now = millis();
    if ((now - lastHostHeartbeatMs) < Config::Serial::heartbeatIntervalMs)
    {
      return;
    }

    lastHostHeartbeatMs = now;
    groundHeartbeatCount++;
    sendGroundTelemetryULong("HEARTBEAT_N", groundHeartbeatCount);
    if (!groundTelemetryEnabled)
    {
      return;
    }

    sendGroundStatusSnapshot();
  }

  bool parseCommand(char *line, CommandView &outCommand)
  {
    if (line == nullptr)
    {
      return false;
    }

    char *firstComma = strchr(line, ',');
    if (firstComma == nullptr)
    {
      return false;
    }

    char *secondComma = strchr(firstComma + 1, ',');
    if (secondComma == nullptr)
    {
      return false;
    }

    char *thirdComma = strchr(secondComma + 1, ',');
    if (thirdComma == nullptr)
    {
      return false;
    }

    *firstComma = '\0';
    *secondComma = '\0';
    *thirdComma = '\0';

    outCommand.type = line;
    outCommand.target = firstComma + 1;
    outCommand.parameter = secondComma + 1;
    outCommand.value = thirdComma + 1;
    return true;
  }

  bool equalsToken(const char *value, const char *expected)
  {
    return value != nullptr && expected != nullptr && strcmp(value, expected) == 0;
  }

  bool isGroundTarget(const CommandView &command)
  {
    return equalsToken(command.target, "GROUND");
  }

  void handleGroundGet(const CommandView &command)
  {
    if (equalsToken(command.parameter, "NONE"))
    {
      sendGroundStatusSnapshot();
      return;
    }

    if (equalsToken(command.parameter, "CURRENT_TIME"))
    {
      char timestamp[21];
      formatIsoTimestamp(currentEpochSeconds(), timestamp, sizeof(timestamp));
      sendGroundTelemetry("CURRENT_TIME", timestamp);
      return;
    }

    if (equalsToken(command.parameter, "HEARTBEAT_N"))
    {
      sendGroundTelemetryULong("HEARTBEAT_N", groundHeartbeatCount);
      return;
    }

    if (equalsToken(command.parameter, "SOURCE"))
    {
      sendGroundTelemetryFlash("SOURCE", clockSourceLabel());
      return;
    }

    if (equalsToken(command.parameter, "TELEMETRY"))
    {
      sendGroundTelemetry("TELEMETRY", groundTelemetryEnabled ? "TRUE" : "FALSE");
      return;
    }

    if (equalsToken(command.parameter, "RADIO"))
    {
      sendGroundTelemetry("RADIO", radioReady ? "READY" : "FAILED");
      return;
    }

    if (equalsToken(command.parameter, "PENDING"))
    {
      sendGroundTelemetry("PENDING", pendingCommand.active ? "TRUE" : "FALSE");
      return;
    }

    if (equalsToken(command.parameter, "CLOCK_SYNC"))
    {
      sendGroundTelemetry("CLOCK_SYNC", clockSynced ? "TRUE" : "FALSE");
      return;
    }

    if (equalsToken(command.parameter, "TX_PACKETS_N"))
    {
      sendGroundTelemetryULong("TX_PACKETS_N", txPacketCount);
      return;
    }

    if (equalsToken(command.parameter, "RX_PACKETS_N"))
    {
      sendGroundTelemetryULong("RX_PACKETS_N", rxPacketCount);
      return;
    }

    if (equalsToken(command.parameter, "DROP_PACKETS_N"))
    {
      sendGroundTelemetryULong("DROP_PACKETS_N", dropPacketCount);
      return;
    }

    if (equalsToken(command.parameter, "LAST_RETRY_N"))
    {
      sendGroundTelemetryULong("LAST_RETRY_N", lastRetryAttempt);
      return;
    }

    sendGroundError("BAD_PARAMETER", command.parameter);
  }

  void handleGroundSet(const CommandView &command)
  {
    if (equalsToken(command.parameter, "TELEMETRY"))
    {
      if (equalsToken(command.value, "ENABLE") || equalsToken(command.value, "TRUE"))
      {
        groundTelemetryEnabled = true;
        sendGroundAck("TELEMETRY");
        sendGroundTelemetry("TELEMETRY", "TRUE");
        return;
      }

      if (equalsToken(command.value, "DISABLE") || equalsToken(command.value, "FALSE"))
      {
        groundTelemetryEnabled = false;
        sendGroundAck("TELEMETRY");
        sendGroundTelemetry("TELEMETRY", "FALSE");
        return;
      }

      sendGroundError("BAD_VALUE", command.value);
      return;
    }

    if (!equalsToken(command.parameter, "CURRENT_TIME"))
    {
      sendGroundError("BAD_PARAMETER", command.parameter);
      return;
    }

    uint32_t epochSeconds = 0;
    if (!parseIsoTimestamp(command.value, epochSeconds))
    {
      sendGroundError("BAD_VALUE", command.value);
      return;
    }

    clockBaseEpochSeconds = epochSeconds;
    clockBaseMillis = millis();
    clockSynced = true;
    clockSource = CLOCK_SOURCE_LOCAL;
    sendGroundAck("CLOCK_SET");
    char timestamp[21];
    formatIsoTimestamp(currentEpochSeconds(), timestamp, sizeof(timestamp));
    sendGroundTelemetry("CURRENT_TIME", timestamp);
    sendGroundTelemetryFlash("SOURCE", F("LOCAL"));
    sendGroundTelemetryFlash("CLOCK_SYNC", F("TRUE"));
  }

  void handleGroundPing(const CommandView &command)
  {
    if (!equalsToken(command.parameter, "NONE") || !equalsToken(command.value, "NONE"))
    {
      sendGroundError("BAD_FORMAT", "PING");
      return;
    }

    sendGroundAck("PONG");
  }

  void handleGroundReset(const CommandView &command)
  {
    if (!equalsToken(command.parameter, "NONE") || !equalsToken(command.value, "NONE"))
    {
      sendGroundError("BAD_FORMAT", "RESET");
      return;
    }

    sendGroundAck("REBOOT");
    delay(Config::Protocol::resetAckDelayMs);
    NVIC_SystemReset();
  }

  bool handleGroundCommand(const char *line)
  {
    if (line == nullptr)
    {
      return false;
    }

    char localBuffer[kLineBufferSize];
    strncpy(localBuffer, line, sizeof(localBuffer) - 1);
    localBuffer[sizeof(localBuffer) - 1] = '\0';

    CommandView command{};
    if (!parseCommand(localBuffer, command))
    {
      return false;
    }

    if (!isGroundTarget(command))
    {
      return false;
    }

    if (equalsToken(command.type, "GET"))
    {
      handleGroundGet(command);
      return true;
    }

    if (equalsToken(command.type, "SET"))
    {
      handleGroundSet(command);
      return true;
    }

    if (equalsToken(command.type, "PING"))
    {
      handleGroundPing(command);
      return true;
    }

    if (equalsToken(command.type, "RESET"))
    {
      handleGroundReset(command);
      return true;
    }

    sendGroundError("UNKNOWN_CMD", command.type);
    return true;
  }

  void handleHostSerial()
  {
    while (Serial.available() > 0)
    {
      const char ch = static_cast<char>(Serial.read());

      if (ch == '\r')
      {
        continue;
      }

      if (ch == '\n')
      {
        hostInputBuffer[hostInputLength] = '\0';
        if (hostInputLength > 0)
        {
          if (!handleGroundCommand(hostInputBuffer))
          {
            if (sendPayloadToSatellite(hostInputBuffer))
            {
              startPendingCommand(hostInputBuffer);
            }
            else
            {
              sendGroundError("LINK_DOWN", hostInputBuffer);
            }
          }
        }
        hostInputLength = 0;
        continue;
      }

      if (hostInputLength < (sizeof(hostInputBuffer) - 1))
      {
        hostInputBuffer[hostInputLength++] = ch;
      }
      else
      {
        hostInputLength = 0;
      }
    }
  }

  void handleRadioReceive()
  {
    const int packetLength = LoRa.parsePacket();
    if (packetLength <= 0)
    {
      return;
    }

    if (packetLength > static_cast<int>(kPacketBufferSize))
    {
      while (LoRa.available())
      {
        LoRa.read();
      }
      dropPacketCount++;
      emitDropTelemetry(RfEnvelope::DECODE_PAYLOAD_TOO_LARGE);
      return;
    }

    uint8_t packetBuffer[kPacketBufferSize];
    size_t bytesRead = 0;
    while (LoRa.available() && bytesRead < static_cast<size_t>(packetLength))
    {
      const int value = LoRa.read();
      if (value < 0)
      {
        break;
      }

      packetBuffer[bytesRead++] = static_cast<uint8_t>(value);
    }

    RfEnvelope::DecodedPacket decodedPacket{};
    const RfEnvelope::DecodeStatus decodeStatus =
        RfEnvelope::decodePacket(packetBuffer,
                                 bytesRead,
                                 RfEnvelope::groundStationDeviceId,
                                 decodedPacket);
    if (decodeStatus != RfEnvelope::DECODE_OK)
    {
      dropPacketCount++;
      emitDropTelemetry(decodeStatus);
      return;
    }

    maybeSyncClockFromSatellitePayload(decodedPacket.payload);
    if (shouldSuppressDuplicatePayload(decodedPacket.payload))
    {
      return;
    }

    rxPacketCount++;
    forwardPayloadToHost(decodedPacket.payload);
    rememberForwardedPayload(decodedPacket.payload);
    noteLedActivity();
    if (payloadMatchesPendingResponse(decodedPacket.payload))
    {
      clearPendingCommand();
    }
  }

  void handleCommandRetry()
  {
    if (!pendingCommand.active)
    {
      return;
    }

    const unsigned long now = millis();
    if ((now - pendingCommand.lastSendMs) < Config::Retry::commandRetryDelayMs)
    {
      return;
    }

    if (pendingCommand.retryCount >= Config::Retry::maxCommandRetries)
    {
      sendGroundTelemetryULong("LAST_RETRY_N", lastRetryAttempt);
      sendGroundError("TIMEOUT", pendingCommand.line);
      clearPendingCommand();
      return;
    }

    if (sendPayloadToSatellite(pendingCommand.line))
    {
      pendingCommand.lastSendMs = now;
      pendingCommand.retryCount++;
      lastRetryAttempt = pendingCommand.retryCount;
      sendGroundTelemetryULong("LAST_RETRY_N", lastRetryAttempt);
    }
    else
    {
      sendGroundError("RETRY_SEND_FAILED", pendingCommand.line);
    }
  }
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  ledPulseActive = false;
  ledPulseStartMs = 0;
  applyLedOutput();

  clockBaseEpochSeconds = kUnsyncedEpochSeconds;
  clockBaseMillis = millis();
  clockSynced = false;
  clockSource = CLOCK_SOURCE_UNSYNC;
  groundTelemetryEnabled = Config::Telemetry::defaultEnabled;
  lastForwardedPayload[0] = '\0';
  lastForwardedPayloadMs = 0;

  Serial.begin(Config::Serial::baudRate);
  while (!Serial)
  {
    // Wait for the USB serial port on boards that expose it.
  }

  radioReady = (LoRa.begin(Config::Transport::loraFrequencyHz) == 1);
  if (radioReady)
  {
    LoRa.setTxPower(Config::Transport::loraTxPowerDbm);
    LoRa.setSignalBandwidth(Config::Transport::loraSignalBandwidthHz);
    LoRa.setSpreadingFactor(Config::Transport::loraSpreadingFactor);
    LoRa.setCodingRate4(Config::Transport::loraCodingRateDenominator);
    LoRa.setPreambleLength(Config::Transport::loraPreambleLength);
    LoRa.setSyncWord(Config::Transport::loraSyncWord);
    LoRa.enableCrc();
  }

  sendGroundTelemetry("STARTED", "TRUE");
  sendGroundStatusSnapshot();
  lastHostHeartbeatMs = millis();
}

void loop()
{
  updateLed();
  emitGroundHeartbeat();
  handleHostSerial();
  if (radioReady)
  {
    handleRadioReceive();
    handleCommandRetry();
  }
}
