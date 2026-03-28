#include "telemetry.h"

#include "gps.h"
#include "led.h"
#include "pmic.h"
#include "rtc.h"
#include "status.h"
#include "telemetry_config_internal.h"

namespace
{
  unsigned long lastTelemetryTime = 0;
}

void resetTelemetrySchedule()
{
  lastTelemetryTime = getUptimeSeconds();
}

void sendTelemetrySnapshot()
{
  reportStatusHeartbeat(true);
  if (isTargetTelemetryEnabled(TARGET_RTC))
  {
    reportRtcStatus();
  }
  if (isTargetTelemetryEnabled(TARGET_LED))
  {
    reportLedStatus();
  }
  if (isTargetTelemetryEnabled(TARGET_TELEMETRY))
  {
    reportTelemetryStatus();
  }
  if (isTargetTelemetryEnabled(TARGET_BATTERY))
  {
    reportBatteryStatus();
  }
  if (isTargetTelemetryEnabled(TARGET_GPS))
  {
    reportGpsStatus();
  }
}

void handlePeriodicTelemetry()
{
  const unsigned long now = getUptimeSeconds();

  if ((now - lastTelemetryTime) >= getTelemetryIntervalSecondsInternal())
  {
    lastTelemetryTime = now;

    if (!isTelemetryEnabledInternal())
    {
      reportStatusHeartbeat(true);
      return;
    }

    sendTelemetrySnapshot();
  }
}
