#include "commands.h"

#include <Arduino.h>
#include <string.h>

#include "protocol.h"
#include "rf_envelope.h"
#include "time_utils.h"

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
      sendGroundStatusSnapshot(context.currentEpochSeconds, makeGroundStatusSnapshot(context));
      return;
    }

    if (equalsToken(command.parameter, "CURRENT_TIME"))
    {
      char timestamp[21];
      formatIsoTimestamp(context.currentEpochSeconds, timestamp, sizeof(timestamp));
      sendGroundTelemetry(context.currentEpochSeconds, "CURRENT_TIME", timestamp);
      return;
    }

    if (equalsToken(command.parameter, "HEARTBEAT_N"))
    {
      sendGroundTelemetryULong(context.currentEpochSeconds, "HEARTBEAT_N", context.heartbeatCount);
      return;
    }

    if (equalsToken(command.parameter, "SOURCE"))
    {
      sendGroundTelemetry(context.currentEpochSeconds, "SOURCE", groundClockSourceToken(context.clockSource));
      return;
    }

    if (equalsToken(command.parameter, "TELEMETRY"))
    {
      sendGroundTelemetry(context.currentEpochSeconds, "TELEMETRY", context.telemetryEnabled ? "TRUE" : "FALSE");
      return;
    }

    if (equalsToken(command.parameter, "RADIO"))
    {
      sendGroundTelemetry(context.currentEpochSeconds, "RADIO", context.radioReady ? "READY" : "FAILED");
      return;
    }

    if (equalsToken(command.parameter, "PENDING"))
    {
      sendGroundTelemetry(context.currentEpochSeconds, "PENDING", context.pending ? "TRUE" : "FALSE");
      return;
    }

    if (equalsToken(command.parameter, "CLOCK_SYNC"))
    {
      sendGroundTelemetry(context.currentEpochSeconds, "CLOCK_SYNC", context.clockSynced ? "TRUE" : "FALSE");
      return;
    }

    if (equalsToken(command.parameter, "TX_PACKETS_N"))
    {
      sendGroundTelemetryULong(context.currentEpochSeconds, "TX_PACKETS_N", context.txPacketCount);
      return;
    }

    if (equalsToken(command.parameter, "RX_PACKETS_N"))
    {
      sendGroundTelemetryULong(context.currentEpochSeconds, "RX_PACKETS_N", context.rxPacketCount);
      return;
    }

    if (equalsToken(command.parameter, "DROP_PACKETS_N"))
    {
      sendGroundTelemetryULong(context.currentEpochSeconds, "DROP_PACKETS_N", context.dropPacketCount);
      return;
    }

    if (equalsToken(command.parameter, "LAST_RETRY_N"))
    {
      sendGroundTelemetryULong(context.currentEpochSeconds, "LAST_RETRY_N", context.lastRetryAttempt);
      return;
    }

    sendGroundError(context.currentEpochSeconds, "BAD_PARAMETER", command.parameter);
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
        sendGroundAck(context.currentEpochSeconds, "TELEMETRY");
        sendGroundTelemetry(context.currentEpochSeconds, "TELEMETRY", "TRUE");
        return;
      }

      if (equalsToken(command.value, "DISABLE") || equalsToken(command.value, "FALSE"))
      {
        if (context.telemetryEnabledState != nullptr)
        {
          *context.telemetryEnabledState = false;
        }
        context.telemetryEnabled = false;
        sendGroundAck(context.currentEpochSeconds, "TELEMETRY");
        sendGroundTelemetry(context.currentEpochSeconds, "TELEMETRY", "FALSE");
        return;
      }

      sendGroundError(context.currentEpochSeconds, "BAD_VALUE", command.value);
      return;
    }

    if (!equalsToken(command.parameter, "CURRENT_TIME"))
    {
      sendGroundError(context.currentEpochSeconds, "BAD_PARAMETER", command.parameter);
      return;
    }

    uint32_t epochSeconds = 0;
    if (!parseIsoTimestamp(command.value, epochSeconds))
    {
      sendGroundError(context.currentEpochSeconds, "BAD_VALUE", command.value);
      return;
    }

    if (context.clockBaseEpochSeconds != nullptr)
    {
      *context.clockBaseEpochSeconds = epochSeconds;
    }
    if (context.clockBaseMillis != nullptr)
    {
      *context.clockBaseMillis = millis();
    }
    if (context.clockSyncedState != nullptr)
    {
      *context.clockSyncedState = true;
    }
    if (context.clockSourceState != nullptr)
    {
      *context.clockSourceState = GROUND_CLOCK_SOURCE_LOCAL;
    }

    context.currentEpochSeconds = epochSeconds;
    context.clockSynced = true;
    context.clockSource = GROUND_CLOCK_SOURCE_LOCAL;

    sendGroundAck(context.currentEpochSeconds, "CLOCK_SET");
    char timestamp[21];
    formatIsoTimestamp(context.currentEpochSeconds, timestamp, sizeof(timestamp));
    sendGroundTelemetry(context.currentEpochSeconds, "CURRENT_TIME", timestamp);
    sendGroundTelemetry(context.currentEpochSeconds, "SOURCE", "LOCAL");
    sendGroundTelemetry(context.currentEpochSeconds, "CLOCK_SYNC", "TRUE");
  }

  void handleGroundPing(const CommandView &command, GroundCommandContext &context)
  {
    if (!equalsToken(command.parameter, "NONE") || !equalsToken(command.value, "NONE"))
    {
      sendGroundError(context.currentEpochSeconds, "BAD_FORMAT", "PING");
      return;
    }

    sendGroundAck(context.currentEpochSeconds, "PONG");
  }

  void handleGroundReset(const CommandView &command, GroundCommandContext &context)
  {
    if (!equalsToken(command.parameter, "NONE") || !equalsToken(command.value, "NONE"))
    {
      sendGroundError(context.currentEpochSeconds, "BAD_FORMAT", "RESET");
      return;
    }

    sendGroundAck(context.currentEpochSeconds, "REBOOT");
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

  sendGroundError(context.currentEpochSeconds, "UNKNOWN_CMD", command.type);
  return true;
}
