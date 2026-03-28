#include "gps.h"

#include "gps_internal.h"
#include "sender.h"
#include "telemetry.h"

namespace
{
  void reportGpsTelemetryStatus()
  {
    sendTelemetry("GPS", "TELEMETRY", isTargetTelemetryEnabled(TARGET_GPS) ? "TRUE" : "FALSE");
  }

  void reportGpsEnableStatus()
  {
    sendTelemetry("GPS", "ENABLE", getGpsState().enabled ? "TRUE" : "FALSE");
  }

  void reportGpsAvailability()
  {
    sendTelemetry("GPS", "AVAILABLE", getGpsState().fixAvailable ? "TRUE" : "FALSE");
  }

  void reportGpsLatitude()
  {
    sendTelemetryFloat("GPS", "LATITUDE_D", getGpsState().fixAvailable ? getGpsState().latitudeDeg : 0.0f, getGpsCoordinateDecimalPlaces());
  }

  void reportGpsLongitude()
  {
    sendTelemetryFloat("GPS", "LONGITUDE_D", getGpsState().fixAvailable ? getGpsState().longitudeDeg : 0.0f, getGpsCoordinateDecimalPlaces());
  }

  void reportGpsAltitude()
  {
    sendTelemetryFloat("GPS", "ALTITUDE_M", getGpsState().fixAvailable ? getGpsState().altitudeM : 0.0f, 2);
  }

  void reportGpsSpeed()
  {
    sendTelemetryFloat("GPS", "SPEED_KPH", getGpsState().fixAvailable ? getGpsState().speedKph : 0.0f, 2);
  }

  void reportGpsSatellites()
  {
    sendTelemetryULong("GPS", "SATELLITES_N", getGpsState().fixAvailable ? static_cast<unsigned long>(getGpsState().satellitesVisible) : 0);
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
