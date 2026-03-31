#include "telemetry.h"

#include "adcs.h"
#include "imu.h"
#include "mode.h"
#include "gps.h"
#include "pmic.h"
#include "rtc.h"
#include "sender.h"
#include "status.h"
#include "thermal.h"
#include "telemetry_config_internal.h"
#include "transport.h"

namespace
{
  unsigned long lastTelemetryTime = 0;

  void sendModeBatch()
  {
    beginTelemetryBatch();
    reportModeStatus();
    endTelemetryBatch();
  }

  void sendRtcBatch()
  {
    beginTelemetryBatch();
    reportRtcStatus();
    endTelemetryBatch();
  }

  void sendTelemetryControlBatch()
  {
    beginTelemetryBatch();
    reportTelemetryStatus();
    endTelemetryBatch();
  }

  void sendBatteryBatch()
  {
    beginTelemetryBatch();
    reportBatteryStatus();
    endTelemetryBatch();
  }

  void sendGpsBatch()
  {
    beginTelemetryBatch();
    reportGpsStatus();
    endTelemetryBatch();
  }

  void sendThermalBatch()
  {
    beginTelemetryBatch();
    reportThermalStatus();
    endTelemetryBatch();
  }

  void sendImuBatch()
  {
    beginTelemetryBatch();
    reportImuStatus();
    endTelemetryBatch();
  }

  void sendAdcsBatch()
  {
    beginTelemetryBatch();
    reportAdcsStatus();
    endTelemetryBatch();
  }
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
  sendModeBatch();
  if (isTargetTelemetryEnabled(TARGET_RTC))
  {
    sendRtcBatch();
  }
  if (isTargetTelemetryEnabled(TARGET_TELEMETRY))
  {
    sendTelemetryControlBatch();
  }
  if (isTargetTelemetryEnabled(TARGET_BATTERY))
  {
    sendBatteryBatch();
  }
  if (isTargetTelemetryEnabled(TARGET_GPS))
  {
    sendGpsBatch();
  }
  if (isTargetTelemetryEnabled(TARGET_THERMAL))
  {
    sendThermalBatch();
  }
  if (isTargetTelemetryEnabled(TARGET_IMU))
  {
    sendImuBatch();
  }
  if (isTargetTelemetryEnabled(TARGET_ADCS))
  {
    sendAdcsBatch();
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

    if (transportShouldDeferTelemetry())
    {
      return;
    }

    sendTelemetrySnapshot();
  }
}
