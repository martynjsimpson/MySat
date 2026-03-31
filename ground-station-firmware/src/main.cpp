#include <Arduino.h>
#include <LoRa.h>
#include <string.h>

#include "config.h"
#include "rf_envelope.h"

namespace
{
  constexpr size_t kLineBufferSize = RfEnvelope::maxPayloadLength + 1;
  constexpr size_t kPacketBufferSize = RfEnvelope::maxPacketLength;

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

  char hostInputBuffer[kLineBufferSize];
  size_t hostInputLength = 0;
  unsigned long lastHostHeartbeatMs = 0;
  unsigned long ledPulseStartMs = 0;
  bool ledPulseActive = false;
  unsigned long txPacketCount = 0;
  unsigned long rxPacketCount = 0;
  unsigned long dropPacketCount = 0;

  PendingCommandState pendingCommand;
  bool radioReady = false;

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
    Serial.println(payload);
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

  void emitDiagTx(const char *event, const char *detail = nullptr)
  {
    Serial.print(F("GROUND,DIAG,TX,"));
    Serial.print(event);
    if (detail != nullptr && *detail != '\0')
    {
      Serial.print(F(","));
      Serial.print(detail);
    }
    Serial.print(F(",COUNT,"));
    Serial.println(txPacketCount);
  }

  void emitDiagRx(const char *payload)
  {
    if (!Config::Serial::logRxPayloadDiagnostics)
    {
      return;
    }

    Serial.print(F("GROUND,DIAG,RX,PAYLOAD,"));
    Serial.print(payload);
    Serial.print(F(",COUNT,"));
    Serial.println(rxPacketCount);
  }

  void emitDiagDrop(RfEnvelope::DecodeStatus status)
  {
    Serial.print(F("GROUND,DIAG,DROP,"));
    Serial.print(decodeStatusLabel(status));
    Serial.print(F(",COUNT,"));
    Serial.println(dropPacketCount);
  }

  void emitDiagRetry()
  {
    Serial.print(F("GROUND,DIAG,RETRY,ATTEMPT,"));
    Serial.print(static_cast<unsigned long>(pendingCommand.retryCount + 1));
    Serial.print(F(",COMMAND,"));
    Serial.println(pendingCommand.line);
  }

  void emitDiagTimeout()
  {
    Serial.print(F("GROUND,DIAG,TIMEOUT,COMMAND,"));
    Serial.print(pendingCommand.line);
    Serial.print(F(",RETRIES,"));
    Serial.println(static_cast<unsigned long>(pendingCommand.retryCount));
  }

  void emitHostHeartbeat()
  {
    const unsigned long now = millis();
    if ((now - lastHostHeartbeatMs) < Config::Serial::heartbeatIntervalMs)
    {
      return;
    }

    lastHostHeartbeatMs = now;
    Serial.print(F("GROUND,HEARTBEAT_MS,"));
    Serial.print(now);
    Serial.print(F(",RADIO,"));
    Serial.print(radioReady ? F("READY") : F("FAILED"));
    Serial.print(F(",PENDING,"));
    Serial.print(pendingCommand.active ? F("TRUE") : F("FALSE"));
    Serial.print(F(",TX,"));
    Serial.print(txPacketCount);
    Serial.print(F(",RX,"));
    Serial.print(rxPacketCount);
    Serial.print(F(",DROP,"));
    Serial.println(dropPacketCount);
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
          if (sendPayloadToSatellite(hostInputBuffer))
          {
            startPendingCommand(hostInputBuffer);
            emitDiagTx("COMMAND", hostInputBuffer);
          }
          else
          {
            emitDiagTx("SEND_FAILED", hostInputBuffer);
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
      emitDiagDrop(decodeStatus);
      return;
    }

    rxPacketCount++;
    forwardPayloadToHost(decodedPacket.payload);
    noteLedActivity();
    emitDiagRx(decodedPacket.payload);
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
      emitDiagTimeout();
      clearPendingCommand();
      return;
    }

    emitDiagRetry();
    if (sendPayloadToSatellite(pendingCommand.line))
    {
      pendingCommand.lastSendMs = now;
      pendingCommand.retryCount++;
      emitDiagTx("RETRY", pendingCommand.line);
    }
    else
    {
      emitDiagTx("RETRY_SEND_FAILED", pendingCommand.line);
    }
  }
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  ledPulseActive = false;
  ledPulseStartMs = 0;
  applyLedOutput();

  Serial.begin(Config::Serial::baudRate);
  while (!Serial)
  {
    // Wait for the USB serial port on boards that expose it.
  }

  Serial.println(F("GROUND,STARTED,TRUE"));

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
}

void loop()
{
  updateLed();
  emitHostHeartbeat();
  handleHostSerial();
  if (radioReady)
  {
    handleRadioReceive();
    handleCommandRetry();
  }
}
