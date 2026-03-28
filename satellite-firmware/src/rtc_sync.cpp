#include "rtc.h"

#include "config.h"
#include "gps.h"

namespace
{
  bool autoSyncRtcFromGps = Config::Rtc::defaultAutoSyncFromGps;
  unsigned long lastDriftCheckUptimeSeconds = 0;
  unsigned long lastRtcResyncUptimeSeconds = 0;
  bool driftResyncPending = false;

  void emitRtcSyncTelemetryIfTransitioned(bool wasClockSynchronized)
  {
    if (!wasClockSynchronized && isClockSynchronized())
    {
      reportRtcSyncStatus();
    }
  }

  unsigned long absoluteDifferenceSeconds(unsigned long lhs, unsigned long rhs)
  {
    return lhs > rhs ? (lhs - rhs) : (rhs - lhs);
  }
}

void handleRtcAutoSync()
{
  if (!autoSyncRtcFromGps)
  {
    return;
  }

  const unsigned long now = getUptimeSeconds();
  unsigned long gpsEpochSeconds = 0;
  if (!getGpsTimeUnix(gpsEpochSeconds))
  {
    driftResyncPending = false;
    return;
  }

  const bool wasClockSynchronized = isClockSynchronized();
  if (!wasClockSynchronized)
  {
    if (setCurrentTimeUnix(gpsEpochSeconds))
    {
      lastRtcResyncUptimeSeconds = now;
      emitRtcSyncTelemetryIfTransitioned(wasClockSynchronized);
    }
    return;
  }

  if ((now - lastDriftCheckUptimeSeconds) < Config::Rtc::driftCheckIntervalSeconds)
  {
    return;
  }
  lastDriftCheckUptimeSeconds = now;

  unsigned long rtcEpochSeconds = 0;
  if (!getCurrentTimeUnix(rtcEpochSeconds))
  {
    driftResyncPending = false;
    return;
  }

  const unsigned long driftSeconds = absoluteDifferenceSeconds(rtcEpochSeconds, gpsEpochSeconds);
  if (driftSeconds <= Config::Rtc::driftResyncThresholdSeconds)
  {
    driftResyncPending = false;
    return;
  }

  if ((now - lastRtcResyncUptimeSeconds) < Config::Rtc::minResyncIntervalSeconds)
  {
    driftResyncPending = false;
    return;
  }

  // Require the drift threshold to be exceeded on two scheduled checks before
  // rewriting the RTC so a single second-boundary sample does not trigger a correction.
  if (!driftResyncPending)
  {
    driftResyncPending = true;
    return;
  }

  if (setCurrentTimeUnix(gpsEpochSeconds))
  {
    lastRtcResyncUptimeSeconds = now;
    driftResyncPending = false;
    reportRtcCurrentTime();
  }
}

void emitRtcSyncTelemetryForTransition(bool wasClockSynchronized)
{
  emitRtcSyncTelemetryIfTransitioned(wasClockSynchronized);
}
