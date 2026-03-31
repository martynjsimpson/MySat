#include <Arduino.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "clock.h"
#include "commands.h"
#include "led.h"
#include "link.h"
#include "protocol.h"
#include "radio.h"
#include "rf_envelope.h"
#include "sender.h"

namespace
{
  constexpr size_t kLineBufferSize = RfEnvelope::maxPayloadLength + 1;
  char hostInputBuffer[kLineBufferSize];
  size_t hostInputLength = 0;
  unsigned long lastHostHeartbeatMs = 0;
  unsigned long txPacketCount = 0;
  unsigned long rxPacketCount = 0;
  unsigned long dropPacketCount = 0;
  unsigned long lastRetryAttempt = 0;
  unsigned long groundHeartbeatCount = 0;
  unsigned long lastForwardedPayloadMs = 0;
  char lastForwardedPayload[kLineBufferSize]{};

  char pendingCommandLine[kLineBufferSize]{};
  PendingCommandState pendingCommand{};
  bool groundTelemetryEnabled = Config::Telemetry::defaultEnabled;

  GroundStatusSnapshot makeGroundStatusSnapshot()
  {
    GroundStatusSnapshot snapshot;
    snapshot.heartbeatCount = groundHeartbeatCount;
    snapshot.telemetryEnabled = groundTelemetryEnabled;
    snapshot.clockSource = getClockSource();
    snapshot.radioReady = isGroundRadioReady();
    snapshot.pending = pendingCommand.active;
    snapshot.clockSynced = isClockSynchronized();
    snapshot.txPacketCount = txPacketCount;
    snapshot.rxPacketCount = rxPacketCount;
    snapshot.dropPacketCount = dropPacketCount;
    snapshot.lastRetryAttempt = lastRetryAttempt;
    return snapshot;
  }

  const char *decodeStatusLabel(RfEnvelope::DecodeStatus status)
  {
    switch (status)
    {
    case RfEnvelope::DECODE_PACKET_TOO_SHORT:
      return "PACKET_TOO_SHORT";
    case RfEnvelope::DECODE_UNSUPPORTED_VERSION:
      return "UNSUPPORTED_VERSION";
    case RfEnvelope::DECODE_LENGTH_MISMATCH:
      return "LENGTH_MISMATCH";
    case RfEnvelope::DECODE_CRC_MISMATCH:
      return "CRC_MISMATCH";
    case RfEnvelope::DECODE_NOT_FOR_DEVICE:
      return "NOT_FOR_DEVICE";
    case RfEnvelope::DECODE_PAYLOAD_TOO_LARGE:
      return "PAYLOAD_TOO_LARGE";
    case RfEnvelope::DECODE_OK:
    default:
      return "OK";
    }
  }

  void emitDropTelemetry(RfEnvelope::DecodeStatus status)
  {
    const char *context = lastGroundRadioErrorContext();
    sendError(currentEpochSeconds(), decodeStatusLabel(status), (context != nullptr && *context != '\0') ? context : nullptr);
    sendTelemetry(currentEpochSeconds(), "LAST_ERROR", decodeStatusLabel(status));
    sendTelemetryULong(currentEpochSeconds(), "DROP_PACKETS_N", dropPacketCount);
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
    sendTelemetryULong(currentEpochSeconds(), "HEARTBEAT_N", groundHeartbeatCount);
    if (!groundTelemetryEnabled)
    {
      return;
    }

    sendStatusSnapshot(currentEpochSeconds(), makeGroundStatusSnapshot());
  }

  void performGroundReset()
  {
    delay(Config::Protocol::resetAckDelayMs);
    NVIC_SystemReset();
  }

  bool handleGroundCommand(const char *line)
  {
    GroundCommandContext context;
    context.currentEpochSeconds = currentEpochSeconds();
    context.heartbeatCount = groundHeartbeatCount;
    context.telemetryEnabled = groundTelemetryEnabled;
    context.clockSource = getClockSource();
    context.radioReady = isGroundRadioReady();
    context.pending = pendingCommand.active;
    context.clockSynced = isClockSynchronized();
    context.txPacketCount = txPacketCount;
    context.rxPacketCount = rxPacketCount;
    context.dropPacketCount = dropPacketCount;
    context.lastRetryAttempt = lastRetryAttempt;
    context.telemetryEnabledState = &groundTelemetryEnabled;
    context.performReset = performGroundReset;
    return handleGroundCommandLine(line, context);
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
            if (sendPayloadToSatellite(hostInputBuffer, currentEpochSeconds()))
            {
              txPacketCount++;
              noteLedActivity();
              startPendingCommand(pendingCommand, hostInputBuffer, millis());
            }
            else
            {
              sendError(currentEpochSeconds(), "LINK_DOWN", hostInputBuffer);
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
    RfEnvelope::DecodedPacket decodedPacket{};
    const RfEnvelope::DecodeStatus decodeStatus = receivePacketForGround(decodedPacket);
    if (decodeStatus == RfEnvelope::DECODE_PACKET_TOO_SHORT)
    {
      return;
    }
    if (decodeStatus != RfEnvelope::DECODE_OK)
    {
      dropPacketCount++;
      emitDropTelemetry(decodeStatus);
      return;
    }

    if (trySyncClockFromSatelliteTimestamp(decodedPacket.timestampSeconds))
    {
      char timestamp[21];
      formatPacketTimestamp(decodedPacket.timestampSeconds, timestamp, sizeof(timestamp));
      sendTelemetry(currentEpochSeconds(), "CURRENT_TIME", timestamp);
      sendTelemetry(currentEpochSeconds(), "SOURCE", "SATELLITE");
      sendTelemetry(currentEpochSeconds(), "CLOCK_SYNC", "TRUE");
    }
    if (shouldSuppressDuplicatePayload(decodedPacket.payload,
                                       lastForwardedPayload,
                                       lastForwardedPayloadMs,
                                       millis(),
                                       Config::Receive::duplicateSuppressWindowMs))
    {
      return;
    }

    rxPacketCount++;
    forwardPayloadToHost(decodedPacket.payload, decodedPacket.timestampSeconds);
    rememberForwardedPayload(decodedPacket.payload,
                             lastForwardedPayload,
                             sizeof(lastForwardedPayload),
                             lastForwardedPayloadMs,
                             millis());
    noteLedActivity();
    if (payloadMatchesPendingResponse(pendingCommand, decodedPacket.payload))
    {
      clearPendingCommand(pendingCommand);
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
      sendTelemetryULong(currentEpochSeconds(), "LAST_RETRY_N", lastRetryAttempt);
      sendError(currentEpochSeconds(), "TIMEOUT", pendingCommand.line);
      clearPendingCommand(pendingCommand);
      return;
    }

    if (sendPayloadToSatellite(pendingCommand.line, currentEpochSeconds()))
    {
      txPacketCount++;
      noteLedActivity();
      pendingCommand.lastSendMs = now;
      pendingCommand.retryCount++;
      lastRetryAttempt = pendingCommand.retryCount;
      sendTelemetryULong(currentEpochSeconds(), "LAST_RETRY_N", lastRetryAttempt);
    }
    else
    {
      sendError(currentEpochSeconds(), "RETRY_SEND_FAILED", pendingCommand.line);
    }
  }
}

void setup()
{
  setupLed();
  setupClock();
  groundTelemetryEnabled = Config::Telemetry::defaultEnabled;
  pendingCommand.line = pendingCommandLine;
  pendingCommand.lineCapacity = sizeof(pendingCommandLine);
  clearPendingCommand(pendingCommand);
  lastForwardedPayload[0] = '\0';
  lastForwardedPayloadMs = 0;

  Serial.begin(Config::Serial::baudRate);
  while (!Serial)
  {
    // Wait for the USB serial port on boards that expose it.
  }

  setupGroundRadio();

  sendTelemetry(currentEpochSeconds(), "STARTED", "TRUE");
  sendStatusSnapshot(currentEpochSeconds(), makeGroundStatusSnapshot());
  lastHostHeartbeatMs = millis();
}

void loop()
{
  updateLed();
  emitGroundHeartbeat();
  handleHostSerial();
  if (isGroundRadioReady())
  {
    handleRadioReceive();
    handleCommandRetry();
  }
}
