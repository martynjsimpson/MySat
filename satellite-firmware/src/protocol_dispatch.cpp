#include "protocol_internal.h"

#include <Arduino.h>

#include "config.h"
#include "gps.h"
#include "led.h"
#include "pmic.h"
#include "rtc.h"
#include "sender.h"
#include "status.h"
#include "telemetry.h"

namespace
{
  void handleGet(const Command &cmd)
  {
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

    case TARGET_RTC:
      handleSetRtc(cmd);
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
    Serial.flush();
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
