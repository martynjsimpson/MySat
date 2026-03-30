#include "rtc.h"

#include <string.h>

#include "gps.h"
#include "rtc_sync_internal.h"
#include "sender.h"
#include "telemetry.h"

void reportRtcTelemetryStatus()
{
  sendTelemetry("RTC", "TELEMETRY", isTargetTelemetryEnabled(TARGET_RTC) ? "TRUE" : "FALSE");
}

void reportRtcCurrentTime()
{
  char timestamp[21];
  if (!getCurrentTimestampIso(timestamp, sizeof(timestamp)))
  {
    sendError("RTC_READ_FAILED");
    return;
  }

  sendTelemetry("RTC", "CURRENT_TIME", timestamp);
}

void reportRtcSyncStatus()
{
  sendTelemetry("RTC", "SYNC", isClockSynchronized() ? "TRUE" : "FALSE");
}

void reportRtcSource()
{
  sendTelemetry("RTC", "SOURCE", getRtcSource());
}

void reportRtcStatus()
{
  reportRtcTelemetryStatus();
  reportRtcCurrentTime();
  reportRtcSyncStatus();
  reportRtcSource();
}

void handleGetRtc(const Command &cmd)
{
  if (cmd.value != VALUE_NONE)
  {
    sendError("BAD_FORMAT");
    return;
  }

  switch (cmd.parameter)
  {
  case PARAM_NONE:
    reportRtcStatus();
    break;

  case PARAM_CURRENT_TIME:
    reportRtcCurrentTime();
    break;

  case PARAM_TELEMETRY:
    reportRtcTelemetryStatus();
    break;

  case PARAM_SYNC:
    reportRtcSyncStatus();
    break;

  case PARAM_SOURCE:
    reportRtcSource();
    break;

  default:
    sendError("BAD_PARAMETER");
    break;
  }
}

void handleSetRtc(const Command &cmd)
{
  switch (cmd.parameter)
  {
  case PARAM_CURRENT_TIME:
  {
    const bool wasClockSynchronized = isClockSynchronized();
    if (cmd.rawValueToken == nullptr || !setCurrentTimeIso(cmd.rawValueToken))
    {
      sendError("BAD_VALUE");
      return;
    }

    setRtcSource("LOCAL");
    emitRtcSyncTelemetryForTransition(wasClockSynchronized);
    sendAck("RTC", "CURRENT_TIME");
    break;
  }

  case PARAM_SYNC:
  {
    const bool wasClockSynchronized = isClockSynchronized();
    if (cmd.rawValueToken == nullptr || strcmp(cmd.rawValueToken, "GPS") != 0)
    {
      sendError("BAD_VALUE");
      return;
    }

    unsigned long gpsEpochSeconds = 0;
    if (!getGpsTimeUnix(gpsEpochSeconds) || !setCurrentTimeUnix(gpsEpochSeconds))
    {
      sendError("GPS_TIME_UNAVAILABLE");
      return;
    }

    setRtcSource("GPS");
    emitRtcSyncTelemetryForTransition(wasClockSynchronized);
    sendAck("RTC", "SYNC");
    break;
  }

  default:
    sendError("BAD_PARAMETER");
    break;
  }
}
