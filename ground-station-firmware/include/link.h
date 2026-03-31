#ifndef GROUND_STATION_LINK_H
#define GROUND_STATION_LINK_H

#include <stddef.h>
#include <stdint.h>

#include "protocol.h"

struct PendingCommandState
{
  bool active = false;
  char *line = nullptr;
  size_t lineCapacity = 0;
  unsigned long lastSendMs = 0;
  uint8_t retryCount = 0;
  PendingResponseKind expectedResponse = RESPONSE_NONE;
};

void startPendingCommand(PendingCommandState &state, const char *line, unsigned long nowMs);
void clearPendingCommand(PendingCommandState &state);
bool payloadMatchesPendingResponse(const PendingCommandState &state, const char *payload);

bool formatPacketTimestamp(uint32_t timestampSeconds, char *timestampBuffer, size_t timestampBufferSize);
void forwardPayloadToHost(const char *payload, uint32_t timestampSeconds);

bool shouldSuppressDuplicatePayload(const char *payload,
                                    const char *lastForwardedPayload,
                                    unsigned long lastForwardedPayloadMs,
                                    unsigned long nowMs,
                                    unsigned long suppressWindowMs);
void rememberForwardedPayload(const char *payload,
                              char *lastForwardedPayload,
                              size_t lastForwardedPayloadCapacity,
                              unsigned long &lastForwardedPayloadMs,
                              unsigned long nowMs);

#endif
