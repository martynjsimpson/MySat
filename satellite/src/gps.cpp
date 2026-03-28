#include "gps.h"

#include <Arduino.h>
#include <Arduino_MKRGPS.h>

#include "commands.h"
#include "sender.h"
#include "telemetry.h"

namespace
{
  bool gpsEnabled;
  bool gpsFixAvailable;
  bool gpsInitialized;
  float latitudeDeg = 0.0f;
  float longitudeDeg = 0.0f;
  float altitudeM = 0.0f;
  float speedKph = 0.0f;
  int satellitesVisible = 0;
  unsigned long lastFixMillis = 0;
  unsigned long gpsEpochSeconds = 0;
  bool gpsTimeAvailable = false;

  const unsigned long gpsFixTimeoutMs = 10000;
  const float stationarySpeedThresholdKph = 1.0f;
  const int gpsCoordinateDecimalPlaces = 5; // 1 = ~11 km, 2 = ~1.1 km, 3 = ~110m, 4 = ~11m, 5 = ~1.1m, 6 = ~11cm, 7 = ~1.1cm. >5 = jitter

  void clearGpsValues()
  {
    latitudeDeg = 0.0f;
    longitudeDeg = 0.0f;
    altitudeM = 0.0f;
    speedKph = 0.0f;
    satellitesVisible = 0;
    gpsEpochSeconds = 0;
    gpsTimeAvailable = false;
  }

  void reportGpsTelemetryStatus()
  {
    sendTelemetry("GPS", "TELEMETRY", isTargetTelemetryEnabled(TARGET_GPS) ? "TRUE" : "FALSE");
  }

  void reportGpsEnableStatus()
  {
    sendTelemetry("GPS", "ENABLE", gpsEnabled ? "TRUE" : "FALSE");
  }

  void reportGpsAvailability()
  {
    sendTelemetry("GPS", "AVAILABLE", gpsFixAvailable ? "TRUE" : "FALSE");
  }

  void reportGpsLatitude()
  {
    sendTelemetryFloat("GPS", "LATITUDE_D", gpsFixAvailable ? latitudeDeg : 0.0f, gpsCoordinateDecimalPlaces);
  }

  void reportGpsLongitude()
  {
    sendTelemetryFloat("GPS", "LONGITUDE_D", gpsFixAvailable ? longitudeDeg : 0.0f, gpsCoordinateDecimalPlaces);
  }

  void reportGpsAltitude()
  {
    sendTelemetryFloat("GPS", "ALTITUDE_M", gpsFixAvailable ? altitudeM : 0.0f, 2);
  }

  void reportGpsSpeed()
  {
    sendTelemetryFloat("GPS", "SPEED_KPH", gpsFixAvailable ? speedKph : 0.0f, 2);
  }

  void reportGpsSatellites()
  {
    sendTelemetryULong("GPS", "SATELLITES_N", gpsFixAvailable ? static_cast<unsigned long>(satellitesVisible) : 0);
  }
}

void setupGps()
{
  gpsEnabled = true;
  // The MKR GPS is connected via the I2C cable, matching the working Arduino example.
  gpsInitialized = GPS.begin();
  gpsFixAvailable = false;
  lastFixMillis = 0;
  clearGpsValues();
}

void updateGps()
{
  if (!gpsEnabled || !gpsInitialized)
  {
    gpsFixAvailable = false;
    clearGpsValues();
    return;
  }

  if (GPS.available())
  {
    latitudeDeg = GPS.latitude();
    longitudeDeg = GPS.longitude();
    altitudeM = GPS.altitude();
    speedKph = GPS.speed();
    if (speedKph < stationarySpeedThresholdKph)
    {
      speedKph = 0.0f;
    }
    satellitesVisible = GPS.satellites();
    gpsEpochSeconds = GPS.getTime();
    gpsTimeAvailable = (gpsEpochSeconds != 0);
    lastFixMillis = millis();
    gpsFixAvailable = true;
    return;
  }

  if (gpsFixAvailable && (millis() - lastFixMillis) > gpsFixTimeoutMs)
  {
    gpsFixAvailable = false;
    clearGpsValues();
  }
}

bool getGpsTimeUnix(unsigned long &epochSeconds)
{
  updateGps();

  if (!gpsTimeAvailable)
  {
    return false;
  }

  epochSeconds = gpsEpochSeconds;
  return true;
}

void handleSetGps(const Command &cmd)
{
  switch (cmd.parameter)
  {
  case PARAM_ENABLE:
    if (cmd.value == VALUE_TRUE)
    {
      gpsEnabled = true;
      sendAck("GPS", "ENABLE");
    }
    else if (cmd.value == VALUE_FALSE)
    {
      gpsEnabled = false;
      gpsFixAvailable = false;
      lastFixMillis = 0;
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

void handleGetGps(const Command &cmd)
{
  if (cmd.value != VALUE_NONE)
  {
    sendError("BAD_FORMAT");
    return;
  }

  updateGps();

  switch (cmd.parameter)
  {
  case PARAM_NONE:
    reportGpsStatus();
    break;

  case PARAM_TELEMETRY:
    reportGpsTelemetryStatus();
    break;

  case PARAM_ENABLE:
    reportGpsEnableStatus();
    break;

  case PARAM_AVAILABLE:
    reportGpsAvailability();
    break;

  case PARAM_LATITUDE_D:
    reportGpsLatitude();
    break;

  case PARAM_LONGITUDE_D:
    reportGpsLongitude();
    break;

  case PARAM_ALTITUDE_M:
    reportGpsAltitude();
    break;

  case PARAM_SPEED_KPH:
    reportGpsSpeed();
    break;

  case PARAM_SATELLITES_N:
    reportGpsSatellites();
    break;

  default:
    sendError("BAD_PARAMETER");
    break;
  }
}

void reportGpsStatus()
{
  updateGps();

  reportGpsTelemetryStatus();
  reportGpsEnableStatus();
  reportGpsAvailability();
  reportGpsLatitude();
  reportGpsLongitude();
  reportGpsAltitude();
  reportGpsSpeed();
  reportGpsSatellites();
}
