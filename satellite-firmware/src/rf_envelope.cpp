#include "rf_envelope.h"

#include <string.h>

namespace RfEnvelope
{
  namespace
  {
    uint16_t readBigEndianU16(const uint8_t *data)
    {
      return static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 8) |
                                   static_cast<uint16_t>(data[1]));
    }

    void writeBigEndianU16(uint16_t value, uint8_t *data)
    {
      data[0] = static_cast<uint8_t>((value >> 8) & 0xFF);
      data[1] = static_cast<uint8_t>(value & 0xFF);
    }
  } // namespace

  uint16_t crc16CcittFalse(const uint8_t *data, size_t length)
  {
    uint16_t crc = 0xFFFF;

    for (size_t index = 0; index < length; ++index)
    {
      crc ^= static_cast<uint16_t>(data[index]) << 8;

      for (uint8_t bit = 0; bit < 8; ++bit)
      {
        if ((crc & 0x8000u) != 0u)
        {
          crc = static_cast<uint16_t>((crc << 1) ^ 0x1021u);
        }
        else
        {
          crc = static_cast<uint16_t>(crc << 1);
        }
      }
    }

    return crc;
  }

  size_t encodedPacketLength(size_t payloadLength)
  {
    return overheadSize + payloadLength;
  }

  bool encodePacket(uint8_t source,
                    uint8_t destination,
                    const char *payload,
                    uint8_t *outPacket,
                    size_t outPacketCapacity,
                    size_t &outPacketLength)
  {
    if (payload == nullptr || outPacket == nullptr)
    {
      outPacketLength = 0;
      return false;
    }

    const size_t payloadLength = strlen(payload);
    if (payloadLength > maxPayloadLength)
    {
      outPacketLength = 0;
      return false;
    }

    const size_t packetLength = encodedPacketLength(payloadLength);
    if (outPacketCapacity < packetLength)
    {
      outPacketLength = 0;
      return false;
    }

    outPacket[0] = version;
    outPacket[1] = source;
    outPacket[2] = destination;
    outPacket[3] = static_cast<uint8_t>(payloadLength);

    memcpy(&outPacket[headerSize], payload, payloadLength);

    const uint16_t crc = crc16CcittFalse(outPacket, headerSize + payloadLength);
    writeBigEndianU16(crc, &outPacket[headerSize + payloadLength]);

    outPacketLength = packetLength;
    return true;
  }

  DecodeStatus decodePacket(const uint8_t *packet,
                            size_t packetLength,
                            uint8_t localDeviceId,
                            DecodedPacket &outPacket)
  {
    if (packet == nullptr)
    {
      return DECODE_PACKET_TOO_SHORT;
    }

    if (packetLength < overheadSize)
    {
      return DECODE_PACKET_TOO_SHORT;
    }

    if (packet[0] != version)
    {
      return DECODE_UNSUPPORTED_VERSION;
    }

    const size_t payloadLength = packet[3];
    const size_t expectedLength = encodedPacketLength(payloadLength);
    if (packetLength != expectedLength)
    {
      return DECODE_LENGTH_MISMATCH;
    }

    if (payloadLength > maxPayloadLength)
    {
      return DECODE_PAYLOAD_TOO_LARGE;
    }

    const uint16_t expectedCrc = readBigEndianU16(&packet[headerSize + payloadLength]);
    const uint16_t actualCrc = crc16CcittFalse(packet, headerSize + payloadLength);
    if (expectedCrc != actualCrc)
    {
      return DECODE_CRC_MISMATCH;
    }

    const uint8_t destination = packet[2];
    const bool isForLocalDevice = destination == localDeviceId;
    const bool isBroadcast = destination == broadcastDeviceId;
    if (!isForLocalDevice && !isBroadcast)
    {
      return DECODE_NOT_FOR_DEVICE;
    }

    outPacket.source = packet[1];
    outPacket.destination = destination;
    outPacket.payloadLength = payloadLength;
    memcpy(outPacket.payload, &packet[headerSize], payloadLength);
    outPacket.payload[payloadLength] = '\0';
    return DECODE_OK;
  }
} // namespace RfEnvelope
