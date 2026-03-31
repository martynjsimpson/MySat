#include "protocol.h"

#include <string.h>

namespace
{
  bool payloadTypeEquals(const char *payload, const char *type)
  {
    if (payload == nullptr || type == nullptr)
    {
      return false;
    }

    const char *firstComma = strchr(payload, ',');
    if (firstComma == nullptr)
    {
      return false;
    }

    const size_t typeLength = static_cast<size_t>(firstComma - payload);
    const size_t expectedLength = strlen(type);
    return typeLength == expectedLength && strncmp(payload, type, expectedLength) == 0;
  }
} // namespace

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

bool payloadMatchesExpectedResponse(PendingResponseKind expectedResponse, const char *payload)
{
  if (payload == nullptr)
  {
    return false;
  }

  if (payloadTypeEquals(payload, "ERR"))
  {
    return true;
  }

  if (expectedResponse == RESPONSE_ACK_OR_ERR)
  {
    return payloadTypeEquals(payload, "ACK");
  }

  if (expectedResponse == RESPONSE_TLM_OR_ERR)
  {
    return payloadTypeEquals(payload, "TLM");
  }

  return false;
}
