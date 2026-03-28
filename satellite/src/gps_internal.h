#ifndef GPS_INTERNAL_H
#define GPS_INTERNAL_H

struct GpsState
{
  bool enabled;
  bool fixAvailable;
  bool initialized;
  bool timeAvailable;
  float latitudeDeg;
  float longitudeDeg;
  float altitudeM;
  float speedKph;
  int satellitesVisible;
  unsigned long lastFixMillis;
  unsigned long epochSeconds;
};

const GpsState &getGpsState();
int getGpsCoordinateDecimalPlaces();

#endif
