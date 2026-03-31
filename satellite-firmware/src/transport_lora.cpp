#include "transport.h"

#include <Arduino.h>
#include <LoRa.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "led.h"
#include "rf_envelope.h"

namespace
{
  constexpr size_t kInboundBufferSize = RfEnvelope::maxPayloadLength + 1;
  constexpr size_t kOutboundBufferSize = RfEnvelope::maxPayloadLength + 1;
  constexpr size_t kPacketBufferSize = RfEnvelope::maxPacketLength;

  bool radioReady = false;
  unsigned long lastRadioActivityMs = 0;

  char inboundBuffer[kInboundBufferSize];
  size_t inboundLength = 0;
  size_t inboundPosition = 0;

  char outboundBuffer[kOutboundBufferSize];
  size_t outboundLength = 0;
  uint32_t outboundPacketTimestampSeconds = 0;

  void noteRadioActivity()
  {
    lastRadioActivityMs = millis();
    noteLedActivity();
  }

  bool loadNextInboundPacket()
  {
    if (!radioReady)
    {
      return false;
    }

    const int packetLength = LoRa.parsePacket();
    if (packetLength <= 0)
    {
      return false;
    }

    if (packetLength > static_cast<int>(kPacketBufferSize))
    {
      while (LoRa.available())
      {
        LoRa.read();
      }
      return false;
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
    const RfEnvelope::DecodeStatus status =
        RfEnvelope::decodePacket(packetBuffer,
                                 bytesRead,
                                 RfEnvelope::satelliteDeviceId,
                                 decodedPacket);

    if (status != RfEnvelope::DECODE_OK)
    {
      return false;
    }

    if (decodedPacket.payloadLength >= kInboundBufferSize)
    {
      return false;
    }

    memcpy(inboundBuffer, decodedPacket.payload, decodedPacket.payloadLength);
    inboundLength = decodedPacket.payloadLength;
    inboundBuffer[inboundLength++] = '\n';
    inboundPosition = 0;
    noteRadioActivity();
    return true;
  }

  bool appendOutboundText(const char *text)
  {
    if (text == nullptr)
    {
      return false;
    }

    const size_t textLength = strlen(text);
    if ((outboundLength + textLength) >= kOutboundBufferSize)
    {
      return false;
    }

    memcpy(&outboundBuffer[outboundLength], text, textLength);
    outboundLength += textLength;
    outboundBuffer[outboundLength] = '\0';
    return true;
  }

  bool appendOutboundChar(char value)
  {
    if ((outboundLength + 1) >= kOutboundBufferSize)
    {
      return false;
    }

    outboundBuffer[outboundLength++] = value;
    outboundBuffer[outboundLength] = '\0';
    return true;
  }

  void resetOutboundBuffer()
  {
    outboundLength = 0;
    outboundBuffer[0] = '\0';
    outboundPacketTimestampSeconds = 0;
  }
}

void setupTransport()
{
  inboundLength = 0;
  inboundPosition = 0;
  resetOutboundBuffer();
  lastRadioActivityMs = 0;

  radioReady = (LoRa.begin(Config::Transport::loraFrequencyHz) == 1);
  if (!radioReady)
  {
    return;
  }

  LoRa.setTxPower(Config::Transport::loraTxPowerDbm);
  LoRa.setSignalBandwidth(Config::Transport::loraSignalBandwidthHz);
  LoRa.setSpreadingFactor(Config::Transport::loraSpreadingFactor);
  LoRa.setCodingRate4(Config::Transport::loraCodingRateDenominator);
  LoRa.setPreambleLength(Config::Transport::loraPreambleLength);
  LoRa.setSyncWord(Config::Transport::loraSyncWord);
  LoRa.enableCrc();
}

int transportAvailable()
{
  if (inboundPosition < inboundLength)
  {
    return static_cast<int>(inboundLength - inboundPosition);
  }

  if (loadNextInboundPacket())
  {
    return static_cast<int>(inboundLength - inboundPosition);
  }

  return 0;
}

int transportRead()
{
  if (transportAvailable() <= 0)
  {
    return -1;
  }

  return static_cast<unsigned char>(inboundBuffer[inboundPosition++]);
}

void transportWrite(const char *text)
{
  if (!appendOutboundText(text))
  {
    resetOutboundBuffer();
  }
}

void transportWrite(char value)
{
  if (!appendOutboundChar(value))
  {
    resetOutboundBuffer();
  }
}

void transportWrite(unsigned long value)
{
  char buffer[16];
  ultoa(value, buffer, 10);
  transportWrite(buffer);
}

void transportWrite(float value, int decimals)
{
  char buffer[24];
  snprintf(buffer, sizeof(buffer), "%.*f", decimals, static_cast<double>(value));

  char *trimmed = buffer;
  while (*trimmed == ' ')
  {
    ++trimmed;
  }

  transportWrite(trimmed);
}

void transportSetPacketTimestamp(uint32_t timestampSeconds)
{
  outboundPacketTimestampSeconds = timestampSeconds;
}

void transportWriteLine()
{
  if (!radioReady || outboundLength == 0)
  {
    resetOutboundBuffer();
    return;
  }

  uint8_t packetBuffer[kPacketBufferSize];
  size_t packetLength = 0;
  if (!RfEnvelope::encodePacket(RfEnvelope::satelliteDeviceId,
                                RfEnvelope::groundStationDeviceId,
                                outboundPacketTimestampSeconds,
                                outboundBuffer,
                                packetBuffer,
                                sizeof(packetBuffer),
                                packetLength))
  {
    resetOutboundBuffer();
    return;
  }

  if (LoRa.beginPacket() != 1)
  {
    resetOutboundBuffer();
    return;
  }

  LoRa.write(packetBuffer, packetLength);
  if (LoRa.endPacket(false) == 1)
  {
    noteRadioActivity();
  }
  resetOutboundBuffer();
}

void transportFlush()
{
}

bool transportShouldDeferTelemetry()
{
  if (!radioReady)
  {
    return false;
  }

  if (inboundPosition < inboundLength)
  {
    return true;
  }

  if (loadNextInboundPacket())
  {
    return true;
  }

  if (lastRadioActivityMs == 0)
  {
    return false;
  }

  return (millis() - lastRadioActivityMs) < Config::Transport::telemetryDeferAfterActivityMs;
}
