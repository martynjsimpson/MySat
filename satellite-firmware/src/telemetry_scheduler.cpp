#include "telemetry.h"

#include "gps.h"
#include "led.h"
#include "pmic.h"
#include "rtc.h"
#include "status.h"
#include "thermal.h"
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
  // STATUS heartbeat is always emitted so the ground side can distinguish
  // "telemetry disabled" from "link appears dead".
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
  if (isTargetTelemetryEnabled(TARGET_THERMAL))
  {
    reportThermalStatus();
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
      // Even with normal telemetry disabled, the heartbeat remains the liveness signal.
      reportStatusHeartbeat(true);
      return;
    }

    sendTelemetrySnapshot();
  }
}
