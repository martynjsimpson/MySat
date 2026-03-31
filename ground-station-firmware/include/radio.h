#ifndef GROUND_STATION_RADIO_H
#define GROUND_STATION_RADIO_H

#include "rf_envelope.h"

bool setupGroundRadio();
bool isGroundRadioReady();
bool sendPayloadToSatellite(const char *payload, uint32_t timestampSeconds);
RfEnvelope::DecodeStatus receivePacketForGround(RfEnvelope::DecodedPacket &outPacket);

#endif
