#include <Arduino.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "commands.h"
#include "link.h"
#include "sender.h"
#include "protocol.h"
#include "radio.h"
#include "rf_envelope.h"
#include "time_utils.h"

namespace
{
  constexpr size_t kLineBufferSize = RfEnvelope::maxPayloadLength + 1;
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

  char pendingCommandLine[kLineBufferSize]{};
  PendingCommandState pendingCommand{};
  uint32_t clockBaseEpochSeconds = kUnsyncedEpochSeconds;
  unsigned long clockBaseMillis = 0;
  bool clockSynced = false;
  GroundClockSource clockSource = GROUND_CLOCK_SOURCE_UNSYNC;
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

  uint32_t currentEpochSeconds()
  {
    return clockBaseEpochSeconds + ((millis() - clockBaseMillis) / 1000UL);
  }

  GroundStatusSnapshot makeGroundStatusSnapshot()
  {
    GroundStatusSnapshot snapshot;
    snapshot.heartbeatCount = groundHeartbeatCount;
    snapshot.telemetryEnabled = groundTelemetryEnabled;
    snapshot.clockSource = clockSource;
    snapshot.radioReady = isGroundRadioReady();
    snapshot.pending = pendingCommand.active;
    snapshot.clockSynced = clockSynced;
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
    sendGroundTelemetry(currentEpochSeconds(), "LAST_ERROR", decodeStatusLabel(status));
    sendGroundTelemetryULong(currentEpochSeconds(), "DROP_PACKETS_N", dropPacketCount);
  }

  bool isTrustedSatelliteTimestamp(const char *timestamp)
  {
    if (timestamp == nullptr)
    {
      return false;
    }

    return strcmp(timestamp, Config::Clock::minTrustedSatelliteTimestamp) >= 0;
  }

  void maybeSyncClockFromSatelliteTimestamp(uint32_t timestampSeconds)
  {
    if (!Config::Clock::autoSyncFromSatellite || clockSource != GROUND_CLOCK_SOURCE_UNSYNC)
    {
      return;
    }

    char timestamp[21];
    formatPacketTimestamp(timestampSeconds, timestamp, sizeof(timestamp));

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
    clockSource = GROUND_CLOCK_SOURCE_SATELLITE;

    sendGroundTelemetry(currentEpochSeconds(), "CURRENT_TIME", timestamp);
    sendGroundTelemetry(currentEpochSeconds(), "SOURCE", "SATELLITE");
    sendGroundTelemetry(currentEpochSeconds(), "CLOCK_SYNC", "TRUE");
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
    sendGroundTelemetryULong(currentEpochSeconds(), "HEARTBEAT_N", groundHeartbeatCount);
    if (!groundTelemetryEnabled)
    {
      return;
    }

    sendGroundStatusSnapshot(currentEpochSeconds(), makeGroundStatusSnapshot());
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
    context.clockSource = clockSource;
    context.radioReady = isGroundRadioReady();
    context.pending = pendingCommand.active;
    context.clockSynced = clockSynced;
    context.txPacketCount = txPacketCount;
    context.rxPacketCount = rxPacketCount;
    context.dropPacketCount = dropPacketCount;
    context.lastRetryAttempt = lastRetryAttempt;
    context.clockBaseEpochSeconds = &clockBaseEpochSeconds;
    context.clockBaseMillis = &clockBaseMillis;
    context.clockSyncedState = &clockSynced;
    context.clockSourceState = &clockSource;
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
              sendGroundError(currentEpochSeconds(), "LINK_DOWN", hostInputBuffer);
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

    maybeSyncClockFromSatelliteTimestamp(decodedPacket.timestampSeconds);
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
      sendGroundTelemetryULong(currentEpochSeconds(), "LAST_RETRY_N", lastRetryAttempt);
      sendGroundError(currentEpochSeconds(), "TIMEOUT", pendingCommand.line);
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
      sendGroundTelemetryULong(currentEpochSeconds(), "LAST_RETRY_N", lastRetryAttempt);
    }
    else
    {
      sendGroundError(currentEpochSeconds(), "RETRY_SEND_FAILED", pendingCommand.line);
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
  clockSource = GROUND_CLOCK_SOURCE_UNSYNC;
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

  sendGroundTelemetry(currentEpochSeconds(), "STARTED", "TRUE");
  sendGroundStatusSnapshot(currentEpochSeconds(), makeGroundStatusSnapshot());
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
