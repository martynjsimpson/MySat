#ifndef RF_ENVELOPE_H
#define RF_ENVELOPE_H

#include <stddef.h>
#include <stdint.h>

namespace RfEnvelope
{
  constexpr uint8_t version = 0x02;

  constexpr uint8_t satelliteDeviceId = 0x01;
  constexpr uint8_t groundStationDeviceId = 0x02;
  constexpr uint8_t broadcastDeviceId = 0xFF;

  constexpr size_t headerSize = 8;
  constexpr size_t crcSize = 2;
  constexpr size_t overheadSize = headerSize + crcSize;
  constexpr size_t maxPayloadLength = 245;
  constexpr size_t maxPacketLength = overheadSize + maxPayloadLength;

  enum DecodeStatus
  {
    DECODE_OK = 0,
    DECODE_PACKET_TOO_SHORT,
    DECODE_UNSUPPORTED_VERSION,
    DECODE_LENGTH_MISMATCH,
    DECODE_CRC_MISMATCH,
    DECODE_NOT_FOR_DEVICE,
    DECODE_PAYLOAD_TOO_LARGE
  };

  struct DecodedPacket
  {
    uint8_t source;
    uint8_t destination;
    uint32_t timestampSeconds;
    size_t payloadLength;
    char payload[maxPayloadLength + 1];
  };

  uint16_t crc16CcittFalse(const uint8_t *data, size_t length);
  size_t encodedPacketLength(size_t payloadLength);
  bool encodePacket(uint8_t source,
                    uint8_t destination,
                    uint32_t timestampSeconds,
                    const char *payload,
                    uint8_t *outPacket,
                    size_t outPacketCapacity,
                    size_t &outPacketLength);
  DecodeStatus decodePacket(const uint8_t *packet,
                            size_t packetLength,
                            uint8_t localDeviceId,
                            DecodedPacket &outPacket);
} // namespace RfEnvelope

#endif
