#include "protocol_internal.h"

#include <Arduino.h>

#include "adcs.h"
#include "imu.h"
#include "config.h"
#include "gps.h"
#include "led.h"
#include "pmic.h"
#include "rtc.h"
#include "sender.h"
#include "status.h"
#include "thermal.h"
#include "telemetry.h"
#include "transport.h"

namespace
{
  void handleSystemWideGet(const Command &cmd)
  {
    if (cmd.parameter != PARAM_NONE || cmd.value != VALUE_NONE)
    {
      sendError("BAD_FORMAT");
      return;
    }

    handleGetTelemetry({CMD_GET, TARGET_TELEMETRY, PARAM_NONE, VALUE_NONE, "NONE", 0, false});
    handleGetRtc({CMD_GET, TARGET_RTC, PARAM_NONE, VALUE_NONE, "NONE", 0, false});
    handleGetLed({CMD_GET, TARGET_LED, PARAM_NONE, VALUE_NONE, "NONE", 0, false});
    handleGetBattery({CMD_GET, TARGET_BATTERY, PARAM_NONE, VALUE_NONE, "NONE", 0, false});
    handleGetGps({CMD_GET, TARGET_GPS, PARAM_NONE, VALUE_NONE, "NONE", 0, false});
    handleGetThermal({CMD_GET, TARGET_THERMAL, PARAM_NONE, VALUE_NONE, "NONE", 0, false});
    handleGetImu({CMD_GET, TARGET_IMU, PARAM_NONE, VALUE_NONE, "NONE", 0, false});
    handleGetAdcs({CMD_GET, TARGET_ADCS, PARAM_NONE, VALUE_NONE, "NONE", 0, false});
    reportStatusHeartbeat(false);
  }

  void handleGet(const Command &cmd)
  {
    if (cmd.target == TARGET_NONE)
    {
      handleSystemWideGet(cmd);
      return;
    }

    switch (cmd.target)
    {
    case TARGET_LED:
      handleGetLed(cmd);
      break;

    case TARGET_TELEMETRY:
      handleGetTelemetry(cmd);
      break;

    case TARGET_BATTERY:
      handleGetBattery(cmd);
      break;

    case TARGET_GPS:
      handleGetGps(cmd);
      break;

    case TARGET_RTC:
      handleGetRtc(cmd);
      break;

    case TARGET_THERMAL:
      handleGetThermal(cmd);
      break;

    case TARGET_IMU:
      handleGetImu(cmd);
      break;

    case TARGET_ADCS:
      handleGetAdcs(cmd);
      break;

    case TARGET_STATUS:
      if ((cmd.parameter != PARAM_NONE && cmd.parameter != PARAM_HEARTBEAT_N) || cmd.value != VALUE_NONE)
      {
        sendError("BAD_FORMAT");
        return;
      }
      reportStatusHeartbeat(false);
      break;

    default:
      sendError("UNKNOWN_TARGET");
      break;
    }
  }

  void handleSet(const Command &cmd)
  {
    if (cmd.parameter == PARAM_TELEMETRY)
    {
      handleSetTargetTelemetry(cmd);
      return;
    }

    switch (cmd.target)
    {
    case TARGET_LED:
      handleSetLed(cmd);
      break;

    case TARGET_GPS:
      handleSetGps(cmd);
      break;

    case TARGET_TELEMETRY:
      handleSetTelemetry(cmd);
      break;

    case TARGET_BATTERY:
      handleSetBattery(cmd);
      break;

    case TARGET_RTC:
      handleSetRtc(cmd);
      break;

    case TARGET_THERMAL:
      handleSetThermal(cmd);
      break;

    case TARGET_IMU:
      handleSetImu(cmd);
      break;

    case TARGET_ADCS:
      handleSetAdcs(cmd);
      break;

    default:
      sendError("UNKNOWN_TARGET");
      break;
    }
  }

  void handlePing(const Command &cmd)
  {
    if (cmd.target == TARGET_NONE &&
        cmd.parameter == PARAM_NONE &&
        cmd.value == VALUE_NONE)
    {
      sendAck("PING", "PONG");
    }
    else
    {
      sendError("BAD_FORMAT");
    }
  }

  void handleReset(const Command &cmd)
  {
    if (cmd.target != TARGET_NONE ||
        cmd.parameter != PARAM_NONE ||
        cmd.value != VALUE_NONE)
    {
      sendError("BAD_FORMAT");
      return;
    }

    sendAck("RESET", "REBOOT");
    // Give the current transport a chance to push the ACK out before the MCU resets.
    transportFlush();
    delay(Config::Protocol::resetAckDelayMs);
    NVIC_SystemReset();
  }
}

void executeCommand(const Command &cmd)
{
  switch (cmd.type)
  {
  case CMD_SET:
    handleSet(cmd);
    break;

  case CMD_GET:
    handleGet(cmd);
    break;

  case CMD_PING:
    handlePing(cmd);
    break;

  case CMD_RESET:
    handleReset(cmd);
    break;

  default:
    sendError("UNKNOWN_CMD");
    break;
  }
}
