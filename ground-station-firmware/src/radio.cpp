#include "radio.h"

#include <Arduino.h>
#include <LoRa.h>
#include <stdio.h>

#include "config.h"

namespace
{
  constexpr size_t kPacketBufferSize = RfEnvelope::maxPacketLength;

  bool radioReady = false;
  char lastErrorContext[32]{};

  void clearLastErrorContext()
  {
    lastErrorContext[0] = '\0';
  }

  void setUnsupportedVersionContext(uint8_t version)
  {
    snprintf(lastErrorContext, sizeof(lastErrorContext), "VER_0x%02X", version);
  }

  void setLengthMismatchContext(size_t actualLength, size_t expectedLength)
  {
    snprintf(lastErrorContext, sizeof(lastErrorContext), "LEN_%u_EXP_%u",
             static_cast<unsigned>(actualLength),
             static_cast<unsigned>(expectedLength));
  }
}

bool setupGroundRadio()
{
  radioReady = (LoRa.begin(Config::Transport::loraFrequencyHz) == 1);
  if (!radioReady)
  {
    return false;
  }

  LoRa.setTxPower(Config::Transport::loraTxPowerDbm);
  LoRa.setSignalBandwidth(Config::Transport::loraSignalBandwidthHz);
  LoRa.setSpreadingFactor(Config::Transport::loraSpreadingFactor);
  LoRa.setCodingRate4(Config::Transport::loraCodingRateDenominator);
  LoRa.setPreambleLength(Config::Transport::loraPreambleLength);
  LoRa.setSyncWord(Config::Transport::loraSyncWord);
  LoRa.enableCrc();
  return true;
}

bool isGroundRadioReady()
{
  return radioReady;
}

bool sendPayloadToSatellite(const char *payload, uint32_t timestampSeconds)
{
  if (!radioReady || payload == nullptr || *payload == '\0')
  {
    return false;
  }

  uint8_t packetBuffer[kPacketBufferSize];
  size_t packetLength = 0;
  if (!RfEnvelope::encodePacket(RfEnvelope::groundStationDeviceId,
                                RfEnvelope::satelliteDeviceId,
                                timestampSeconds,
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
  return LoRa.endPacket(false) == 1;
}

RfEnvelope::DecodeStatus receivePacketForGround(RfEnvelope::DecodedPacket &outPacket)
{
  clearLastErrorContext();

  if (!radioReady)
  {
    return RfEnvelope::DECODE_PACKET_TOO_SHORT;
  }

  const int packetLength = LoRa.parsePacket();
  if (packetLength <= 0)
  {
    return RfEnvelope::DECODE_PACKET_TOO_SHORT;
  }

  if (packetLength > static_cast<int>(kPacketBufferSize))
  {
    while (LoRa.available())
    {
      LoRa.read();
    }
    return RfEnvelope::DECODE_PAYLOAD_TOO_LARGE;
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

  const RfEnvelope::DecodeStatus status =
      RfEnvelope::decodePacket(packetBuffer,
                               bytesRead,
                               RfEnvelope::groundStationDeviceId,
                               outPacket);

  if (status == RfEnvelope::DECODE_UNSUPPORTED_VERSION && bytesRead > 0)
  {
    setUnsupportedVersionContext(packetBuffer[0]);
  }
  else if (status == RfEnvelope::DECODE_LENGTH_MISMATCH && bytesRead >= 4)
  {
    const size_t expectedLength = RfEnvelope::overheadSize + static_cast<size_t>(packetBuffer[3]);
    setLengthMismatchContext(bytesRead, expectedLength);
  }

  return status;
}

const char *lastGroundRadioErrorContext()
{
  return lastErrorContext;
}
