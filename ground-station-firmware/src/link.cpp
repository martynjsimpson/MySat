#include "link.h"

#include <Arduino.h>
#include <string.h>

#include "clock.h"

void startPendingCommand(PendingCommandState &state, const char *line, unsigned long nowMs)
{
  if (line == nullptr || state.line == nullptr || state.lineCapacity == 0)
  {
    state.active = false;
    return;
  }

  strncpy(state.line, line, state.lineCapacity - 1);
  state.line[state.lineCapacity - 1] = '\0';
  state.lastSendMs = nowMs;
  state.retryCount = 0;
  state.expectedResponse = expectedResponseForCommand(line);
  state.active = state.expectedResponse != RESPONSE_NONE;
}

void clearPendingCommand(PendingCommandState &state)
{
  state.active = false;
  if (state.line != nullptr && state.lineCapacity > 0)
  {
    state.line[0] = '\0';
  }
  state.lastSendMs = 0;
  state.retryCount = 0;
  state.expectedResponse = RESPONSE_NONE;
}

bool payloadMatchesPendingResponse(const PendingCommandState &state, const char *payload)
{
  if (!state.active || payload == nullptr)
  {
    return false;
  }

  return payloadMatchesExpectedResponse(state.expectedResponse, payload);
}

void forwardPayloadToHost(const char *payload, uint32_t timestampSeconds)
{
  if (payload == nullptr)
  {
    return;
  }

  char timestamp[21];
  formatPacketTimestamp(timestampSeconds, timestamp, sizeof(timestamp));

  const char *lineStart = payload;
  while (*lineStart != '\0')
  {
    const char *lineEnd = strchr(lineStart, '\n');
    if (lineEnd == nullptr)
    {
      Serial.print(timestamp);
      Serial.print(',');
      Serial.println(lineStart);
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
}

bool shouldSuppressDuplicatePayload(const char *payload,
                                    const char *lastForwardedPayload,
                                    unsigned long lastForwardedPayloadMs,
                                    unsigned long nowMs,
                                    unsigned long suppressWindowMs)
{
  if (payload == nullptr || *payload == '\0')
  {
    return false;
  }

  if (lastForwardedPayload == nullptr || lastForwardedPayload[0] == '\0')
  {
    return false;
  }

  if (strcmp(payload, lastForwardedPayload) != 0)
  {
    return false;
  }

  return (nowMs - lastForwardedPayloadMs) <= suppressWindowMs;
}

void rememberForwardedPayload(const char *payload,
                              char *lastForwardedPayload,
                              size_t lastForwardedPayloadCapacity,
                              unsigned long &lastForwardedPayloadMs,
                              unsigned long nowMs)
{
  if (lastForwardedPayload == nullptr || lastForwardedPayloadCapacity == 0)
  {
    return;
  }

  if (payload == nullptr || *payload == '\0')
  {
    lastForwardedPayload[0] = '\0';
    lastForwardedPayloadMs = 0;
    return;
  }

  strncpy(lastForwardedPayload, payload, lastForwardedPayloadCapacity - 1);
  lastForwardedPayload[lastForwardedPayloadCapacity - 1] = '\0';
  lastForwardedPayloadMs = nowMs;
}
