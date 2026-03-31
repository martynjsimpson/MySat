#include "commands.h"

#include <Arduino.h>
#include <string.h>

#include "clock.h"
#include "protocol.h"
#include "rf_envelope.h"
#include "sender.h"

namespace
{
  GroundStatusSnapshot makeGroundStatusSnapshot(const GroundCommandContext &context)
  {
    GroundStatusSnapshot snapshot;
    snapshot.heartbeatCount = context.heartbeatCount;
    snapshot.telemetryEnabled = context.telemetryEnabled;
    snapshot.clockSource = context.clockSource;
    snapshot.radioReady = context.radioReady;
    snapshot.pending = context.pending;
    snapshot.clockSynced = context.clockSynced;
    snapshot.txPacketCount = context.txPacketCount;
    snapshot.rxPacketCount = context.rxPacketCount;
    snapshot.dropPacketCount = context.dropPacketCount;
    snapshot.lastRetryAttempt = context.lastRetryAttempt;
    return snapshot;
  }

  void handleGroundGet(const CommandView &command, GroundCommandContext &context)
  {
    if (equalsToken(command.parameter, "NONE"))
    {
      sendStatusSnapshot(context.currentEpochSeconds, makeGroundStatusSnapshot(context));
      return;
    }

    if (equalsToken(command.parameter, "CURRENT_TIME"))
    {
      char timestamp[21];
      formatIsoTimestamp(context.currentEpochSeconds, timestamp, sizeof(timestamp));
      sendTelemetry(context.currentEpochSeconds, "CURRENT_TIME", timestamp);
      return;
    }

    if (equalsToken(command.parameter, "HEARTBEAT_N"))
    {
      sendTelemetryULong(context.currentEpochSeconds, "HEARTBEAT_N", context.heartbeatCount);
      return;
    }

    if (equalsToken(command.parameter, "SOURCE"))
    {
      sendTelemetry(context.currentEpochSeconds, "SOURCE", clockSourceToken(context.clockSource));
      return;
    }

    if (equalsToken(command.parameter, "TELEMETRY"))
    {
      sendTelemetry(context.currentEpochSeconds, "TELEMETRY", context.telemetryEnabled ? "TRUE" : "FALSE");
      return;
    }

    if (equalsToken(command.parameter, "RADIO"))
    {
      sendTelemetry(context.currentEpochSeconds, "RADIO", context.radioReady ? "READY" : "FAILED");
      return;
    }

    if (equalsToken(command.parameter, "PENDING"))
    {
      sendTelemetry(context.currentEpochSeconds, "PENDING", context.pending ? "TRUE" : "FALSE");
      return;
    }

    if (equalsToken(command.parameter, "CLOCK_SYNC"))
    {
      sendTelemetry(context.currentEpochSeconds, "CLOCK_SYNC", context.clockSynced ? "TRUE" : "FALSE");
      return;
    }

    if (equalsToken(command.parameter, "TX_PACKETS_N"))
    {
      sendTelemetryULong(context.currentEpochSeconds, "TX_PACKETS_N", context.txPacketCount);
      return;
    }

    if (equalsToken(command.parameter, "RX_PACKETS_N"))
    {
      sendTelemetryULong(context.currentEpochSeconds, "RX_PACKETS_N", context.rxPacketCount);
      return;
    }

    if (equalsToken(command.parameter, "DROP_PACKETS_N"))
    {
      sendTelemetryULong(context.currentEpochSeconds, "DROP_PACKETS_N", context.dropPacketCount);
      return;
    }

    if (equalsToken(command.parameter, "LAST_RETRY_N"))
    {
      sendTelemetryULong(context.currentEpochSeconds, "LAST_RETRY_N", context.lastRetryAttempt);
      return;
    }

    sendError(context.currentEpochSeconds, "BAD_PARAMETER", command.parameter);
  }

  void handleGroundSet(const CommandView &command, GroundCommandContext &context)
  {
    if (equalsToken(command.parameter, "TELEMETRY"))
    {
      if (equalsToken(command.value, "ENABLE") || equalsToken(command.value, "TRUE"))
      {
        if (context.telemetryEnabledState != nullptr)
        {
          *context.telemetryEnabledState = true;
        }
        context.telemetryEnabled = true;
        sendAck(context.currentEpochSeconds, "TELEMETRY");
        sendTelemetry(context.currentEpochSeconds, "TELEMETRY", "TRUE");
        return;
      }

      if (equalsToken(command.value, "DISABLE") || equalsToken(command.value, "FALSE"))
      {
        if (context.telemetryEnabledState != nullptr)
        {
          *context.telemetryEnabledState = false;
        }
        context.telemetryEnabled = false;
        sendAck(context.currentEpochSeconds, "TELEMETRY");
        sendTelemetry(context.currentEpochSeconds, "TELEMETRY", "FALSE");
        return;
      }

      sendError(context.currentEpochSeconds, "BAD_VALUE", command.value);
      return;
    }

    if (!equalsToken(command.parameter, "CURRENT_TIME"))
    {
      sendError(context.currentEpochSeconds, "BAD_PARAMETER", command.parameter);
      return;
    }

    if (!setCurrentTimeIso(command.value))
    {
      sendError(context.currentEpochSeconds, "BAD_VALUE", command.value);
      return;
    }

    context.currentEpochSeconds = currentEpochSeconds();
    context.clockSynced = true;
    context.clockSource = CLOCK_SOURCE_LOCAL;

    sendAck(context.currentEpochSeconds, "CLOCK_SET");
    char timestamp[21];
    formatIsoTimestamp(context.currentEpochSeconds, timestamp, sizeof(timestamp));
    sendTelemetry(context.currentEpochSeconds, "CURRENT_TIME", timestamp);
    sendTelemetry(context.currentEpochSeconds, "SOURCE", "LOCAL");
    sendTelemetry(context.currentEpochSeconds, "CLOCK_SYNC", "TRUE");
  }

  void handleGroundPing(const CommandView &command, GroundCommandContext &context)
  {
    if (!equalsToken(command.parameter, "NONE") || !equalsToken(command.value, "NONE"))
    {
      sendError(context.currentEpochSeconds, "BAD_FORMAT", "PING");
      return;
    }

    sendAck(context.currentEpochSeconds, "PONG");
  }

  void handleGroundReset(const CommandView &command, GroundCommandContext &context)
  {
    if (!equalsToken(command.parameter, "NONE") || !equalsToken(command.value, "NONE"))
    {
      sendError(context.currentEpochSeconds, "BAD_FORMAT", "RESET");
      return;
    }

    sendAck(context.currentEpochSeconds, "REBOOT");
    if (context.performReset != nullptr)
    {
      context.performReset();
    }
  }
} // namespace

bool handleGroundCommandLine(const char *line, GroundCommandContext &context)
{
  if (line == nullptr)
  {
    return false;
  }

  char localBuffer[RfEnvelope::maxPayloadLength + 1];
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
    handleGroundGet(command, context);
    return true;
  }

  if (equalsToken(command.type, "SET"))
  {
    handleGroundSet(command, context);
    return true;
  }

  if (equalsToken(command.type, "PING"))
  {
    handleGroundPing(command, context);
    return true;
  }

  if (equalsToken(command.type, "RESET"))
  {
    handleGroundReset(command, context);
    return true;
  }

  sendError(context.currentEpochSeconds, "UNKNOWN_CMD", command.type);
  return true;
}
