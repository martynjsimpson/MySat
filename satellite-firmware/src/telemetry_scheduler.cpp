#include "telemetry.h"

#include "adcs.h"
#include "config.h"
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

  void finishTelemetryBatch()
  {
    endTelemetryBatch();

    if (Config::Transport::telemetryInterBatchGapMs > 0)
    {
      delay(Config::Transport::telemetryInterBatchGapMs);
    }
  }

  void sendModeBatch()
  {
    beginTelemetryBatch();
    reportModeStatus();
    finishTelemetryBatch();
  }

  void sendRtcBatch()
  {
    beginTelemetryBatch();
    reportRtcStatus();
    finishTelemetryBatch();
  }

  void sendTelemetryControlBatch()
  {
    beginTelemetryBatch();
    reportTelemetryStatus();
    finishTelemetryBatch();
  }

  void sendBatteryBatch()
  {
    beginTelemetryBatch();
    reportBatteryStatus();
    finishTelemetryBatch();
  }

  void sendGpsBatch()
  {
    beginTelemetryBatch();
    reportGpsStatus();
    finishTelemetryBatch();
  }

  void sendThermalBatch()
  {
    beginTelemetryBatch();
    reportThermalStatus();
    finishTelemetryBatch();
  }

  void sendImuBatch()
  {
    beginTelemetryBatch();
    reportImuStatus();
    finishTelemetryBatch();
  }

  void sendAdcsBatch()
  {
    beginTelemetryBatch();
    reportAdcsStatus();
    finishTelemetryBatch();
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
  if (isTargetTelemetryEnabled(TARGET_MODE))
  {
    sendModeBatch();
  }
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
  if (isTargetTelemetryEnabled(TARGET_ADCS))
  {
    sendAdcsBatch();
  }
  if (isTargetTelemetryEnabled(TARGET_IMU))
  {
    sendImuBatch();
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
      incrementSkippedTelemetryCountInternal();
      return;
    }

    sendTelemetrySnapshot();
  }
}
