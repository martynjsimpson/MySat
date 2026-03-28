#include "gps.h"

#include <Arduino.h>
#include <Arduino_MKRGPS.h>

#include "config.h"
#include "gps_internal.h"
#include "sender.h"

namespace
{
  GpsState gpsState = {
      Config::Gps::defaultEnabled,
      false,
      false,
      false,
      0.0f,
      0.0f,
      0.0f,
      0.0f,
      0,
      0,
      0};

  void clearGpsValues()
  {
    gpsState.latitudeDeg = 0.0f;
    gpsState.longitudeDeg = 0.0f;
    gpsState.altitudeM = 0.0f;
    gpsState.speedKph = 0.0f;
    gpsState.satellitesVisible = 0;
    gpsState.epochSeconds = 0;
    gpsState.timeAvailable = false;
  }
}

const GpsState &getGpsState()
{
  return gpsState;
}

int getGpsCoordinateDecimalPlaces()
{
  return Config::Gps::coordinateDecimalPlaces;
}

void setupGps()
{
  gpsState.enabled = Config::Gps::defaultEnabled;
  // The MKR GPS is connected via the I2C cable, matching the working Arduino example.
  gpsState.initialized = GPS.begin();
  gpsState.fixAvailable = false;
  gpsState.lastFixMillis = 0;
  clearGpsValues();
}

void updateGps()
{
  if (!gpsState.enabled || !gpsState.initialized)
  {
    gpsState.fixAvailable = false;
    clearGpsValues();
    return;
  }

  if (GPS.available())
  {
    gpsState.latitudeDeg = GPS.latitude();
    gpsState.longitudeDeg = GPS.longitude();
    gpsState.altitudeM = GPS.altitude();
    gpsState.speedKph = GPS.speed();
    if (gpsState.speedKph < Config::Gps::stationarySpeedThresholdKph)
    {
      gpsState.speedKph = 0.0f;
    }
    gpsState.satellitesVisible = GPS.satellites();
    gpsState.epochSeconds = GPS.getTime();
    gpsState.timeAvailable = (gpsState.epochSeconds != 0);
    gpsState.lastFixMillis = millis();
    gpsState.fixAvailable = true;
    return;
  }

  if (gpsState.fixAvailable && (millis() - gpsState.lastFixMillis) > Config::Gps::fixTimeoutMs)
  {
    gpsState.fixAvailable = false;
    clearGpsValues();
  }
}

bool getGpsTimeUnix(unsigned long &epochSeconds)
{
  updateGps();

  if (!gpsState.timeAvailable)
  {
    return false;
  }

  epochSeconds = gpsState.epochSeconds;
  return true;
}

void handleSetGps(const Command &cmd)
{
  switch (cmd.parameter)
  {
  case PARAM_ENABLE:
    if (cmd.value == VALUE_TRUE)
    {
      gpsState.enabled = true;
      sendAck("GPS", "ENABLE");
    }
    else if (cmd.value == VALUE_FALSE)
    {
      gpsState.enabled = false;
      gpsState.fixAvailable = false;
      gpsState.lastFixMillis = 0;
      clearGpsValues();
      sendAck("GPS", "DISABLE");
    }
    else
    {
      sendError("BAD_VALUE");
    }
    break;

  default:
    sendError("BAD_PARAMETER");
    break;
  }
}
