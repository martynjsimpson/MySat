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

  const unsigned long gpsFixTimeoutMs = 10000;

  void clearGpsValues()
  {
    latitudeDeg = 0.0f;
    longitudeDeg = 0.0f;
    altitudeM = 0.0f;
    speedKph = 0.0f;
    satellitesVisible = 0;
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
    satellitesVisible = GPS.satellites();
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

void reportGpsStatus()
{
  updateGps();

  sendTelemetry("GPS", "TELEMETRY", isTargetTelemetryEnabled(TARGET_GPS) ? "TRUE" : "FALSE");
  sendTelemetry("GPS", "ENABLE", gpsEnabled ? "TRUE" : "FALSE");
  sendTelemetry("GPS", "AVAILABLE", gpsFixAvailable ? "TRUE" : "FALSE");
  sendTelemetryFloat("GPS", "LATITUDE_D", gpsFixAvailable ? latitudeDeg : 0.0f, 7);
  sendTelemetryFloat("GPS", "LONGITUDE_D", gpsFixAvailable ? longitudeDeg : 0.0f, 7);
  sendTelemetryFloat("GPS", "ALTITUDE_M", gpsFixAvailable ? altitudeM : 0.0f, 2);
  sendTelemetryFloat("GPS", "SPEED_KPH", gpsFixAvailable ? speedKph : 0.0f, 2);
  sendTelemetryULong("GPS", "SATELLITES_N", gpsFixAvailable ? static_cast<unsigned long>(satellitesVisible) : 0);
}
