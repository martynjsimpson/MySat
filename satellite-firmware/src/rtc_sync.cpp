#include "rtc.h"

#include "config.h"
#include "gps.h"

namespace
{
  bool autoSyncRtcFromGps = Config::Rtc::defaultAutoSyncFromGps;

  void emitRtcSyncTelemetryIfTransitioned(bool wasClockSynchronized)
  {
    if (!wasClockSynchronized && isClockSynchronized())
    {
      reportRtcSyncStatus();
    }
  }
}

void handleRtcAutoSync()
{
  if (!autoSyncRtcFromGps || isClockSynchronized())
  {
    return;
  }

  unsigned long gpsEpochSeconds = 0;
  if (!getGpsTimeUnix(gpsEpochSeconds))
  {
    return;
  }

  const bool wasClockSynchronized = isClockSynchronized();
  if (setCurrentTimeUnix(gpsEpochSeconds))
  {
    emitRtcSyncTelemetryIfTransitioned(wasClockSynchronized);
  }
}

void emitRtcSyncTelemetryForTransition(bool wasClockSynchronized)
{
  emitRtcSyncTelemetryIfTransitioned(wasClockSynchronized);
}
