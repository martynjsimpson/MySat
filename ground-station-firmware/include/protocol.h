#ifndef GROUND_STATION_PROTOCOL_H
#define GROUND_STATION_PROTOCOL_H

#include <stdint.h>

enum PendingResponseKind
{
  RESPONSE_NONE = 0,
  RESPONSE_ACK_OR_ERR,
  RESPONSE_TLM_OR_ERR
};

struct CommandView
{
  const char *type = nullptr;
  const char *target = nullptr;
  const char *parameter = nullptr;
  const char *value = nullptr;
};

bool parseCommand(char *line, CommandView &outCommand);
bool equalsToken(const char *value, const char *expected);
bool isGroundTarget(const CommandView &command);
PendingResponseKind expectedResponseForCommand(const char *line);
bool payloadMatchesExpectedResponse(PendingResponseKind expectedResponse, const char *payload);

#endif
