#include "protocol.h"

#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

#include "commands.h"
#include "gps.h"
#include "led.h"
#include "pmic.h"
#include "rtc.h"
#include "sender.h"
#include "status.h"
#include "telemetry.h"

namespace
{
  const size_t BUFFER_SIZE = 96;
  char inputBuffer[BUFFER_SIZE];
  char rawCommandBuffer[BUFFER_SIZE];
  size_t inputPos = 0;

  bool isNumericToken(const char *token)
  {
    if (token == nullptr || *token == '\0')
    {
      return false;
    }

    for (const char *p = token; *p != '\0'; ++p)
    {
      if (*p < '0' || *p > '9')
      {
        return false;
      }
    }

    return true;
  }

  CommandType parseCommandType(const char *token)
  {
    if (strcmp(token, "SET") == 0)
      return CMD_SET;
    if (strcmp(token, "GET") == 0)
      return CMD_GET;
    if (strcmp(token, "PING") == 0)
      return CMD_PING;
    if (strcmp(token, "RESET") == 0)
      return CMD_RESET;
    if (strcmp(token, "SAVE") == 0)
      return CMD_SAVE;
    return CMD_UNKNOWN;
  }

  TargetType parseTargetType(const char *token)
  {
    if (strcmp(token, "NONE") == 0)
      return TARGET_NONE;
    if (strcmp(token, "LED") == 0)
      return TARGET_LED;
    if (strcmp(token, "TELEMETRY") == 0)
      return TARGET_TELEMETRY;
    if (strcmp(token, "BATTERY") == 0)
      return TARGET_BATTERY;
    if (strcmp(token, "GPS") == 0)
      return TARGET_GPS;
    if (strcmp(token, "RTC") == 0)
      return TARGET_RTC;
    if (strcmp(token, "MODE") == 0)
      return TARGET_MODE;
    if (strcmp(token, "STATUS") == 0)
      return TARGET_STATUS;
    if (strcmp(token, "RADIO") == 0)
      return TARGET_RADIO;
    if (strcmp(token, "POWER") == 0)
      return TARGET_POWER;
    if (strcmp(token, "PAYLOAD") == 0)
      return TARGET_PAYLOAD;
    if (strcmp(token, "THERMAL") == 0)
      return TARGET_THERMAL;
    if (strcmp(token, "LOG") == 0)
      return TARGET_LOG;
    if (strcmp(token, "WATCHDOG") == 0)
      return TARGET_WATCHDOG;
    if (strcmp(token, "UPTIME") == 0)
      return TARGET_UPTIME;
    return TARGET_UNKNOWN;
  }

  ParameterType parseParameterType(const char *token)
  {
    if (strcmp(token, "NONE") == 0)
      return PARAM_NONE;
    if (strcmp(token, "STATE") == 0)
      return PARAM_STATE;
    if (strcmp(token, "ENABLE") == 0)
      return PARAM_ENABLE;
    if (strcmp(token, "MODE") == 0)
      return PARAM_MODE;
    if (strcmp(token, "HEALTH") == 0)
      return PARAM_HEALTH;
    if (strcmp(token, "INTERVAL_S") == 0)
      return PARAM_INTERVAL_S;
    if (strcmp(token, "TELEMETRY") == 0)
      return PARAM_TELEMETRY;
    if (strcmp(token, "CURRENT_TIME") == 0)
      return PARAM_CURRENT_TIME;
    if (strcmp(token, "HEARTBEAT_N") == 0)
      return PARAM_HEARTBEAT_N;
    if (strcmp(token, "SYNC") == 0)
      return PARAM_SYNC;
    if (strcmp(token, "SECONDS") == 0)
      return PARAM_SECONDS;
    if (strcmp(token, "COLOR") == 0)
      return PARAM_COLOR;
    if (strcmp(token, "AVAILABLE") == 0)
      return PARAM_AVAILABLE;
    if (strcmp(token, "CHARGE_CURRENT_A") == 0)
      return PARAM_CHARGE_CURRENT_A;
    if (strcmp(token, "CHARGE_VOLTAGE_V") == 0)
      return PARAM_CHARGE_VOLTAGE_V;
    if (strcmp(token, "CHARGE_PERCENT_P") == 0)
      return PARAM_CHARGE_PERCENT_P;
    if (strcmp(token, "VOLTAGE_V") == 0)
      return PARAM_VOLTAGE_V;
    if (strcmp(token, "LATITUDE_D") == 0)
      return PARAM_LATITUDE_D;
    if (strcmp(token, "LONGITUDE_D") == 0)
      return PARAM_LONGITUDE_D;
    if (strcmp(token, "ALTITUDE_M") == 0)
      return PARAM_ALTITUDE_M;
    if (strcmp(token, "SPEED_KPH") == 0)
      return PARAM_SPEED_KPH;
    if (strcmp(token, "SATELLITES_N") == 0)
      return PARAM_SATELLITES_N;
    return PARAM_UNKNOWN;
  }

  ValueType parseValueType(const char *token)
  {
    if (strcmp(token, "ON") == 0)
      return VALUE_ON;
    if (strcmp(token, "OFF") == 0)
      return VALUE_OFF;
    if (strcmp(token, "NONE") == 0)
      return VALUE_NONE;
    if (strcmp(token, "TRUE") == 0)
      return VALUE_TRUE;
    if (strcmp(token, "FALSE") == 0)
      return VALUE_FALSE;
    if (strcmp(token, "ENABLE") == 0)
      return VALUE_ENABLE;
    if (strcmp(token, "DISABLE") == 0)
      return VALUE_DISABLE;
    if (strcmp(token, "SAFE") == 0)
      return VALUE_SAFE;
    if (strcmp(token, "NORMAL") == 0)
      return VALUE_NORMAL;
    if (strcmp(token, "LOW_POWER") == 0)
      return VALUE_LOW_POWER;
    if (strcmp(token, "ACTIVE") == 0)
      return VALUE_ACTIVE;
    if (strcmp(token, "IDLE") == 0)
      return VALUE_IDLE;
    if (strcmp(token, "OK") == 0)
      return VALUE_OK;
    if (strcmp(token, "FAIL") == 0)
      return VALUE_FAIL;
    if (strcmp(token, "RED") == 0)
      return VALUE_RED;
    if (strcmp(token, "GREEN") == 0)
      return VALUE_GREEN;
    if (strcmp(token, "BLUE") == 0)
      return VALUE_BLUE;
    return VALUE_UNKNOWN;
  }

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

    default:
      sendError("UNKNOWN_CMD");
      break;
    }
  }

  void processCommand(char *line)
  {
    strncpy(rawCommandBuffer, line, BUFFER_SIZE);
    rawCommandBuffer[BUFFER_SIZE - 1] = '\0';
    setErrorContext(rawCommandBuffer);

    char *cmdToken = strtok(line, ",");
    char *targetToken = strtok(nullptr, ",");
    char *parameterToken = strtok(nullptr, ",");
    char *valueToken = strtok(nullptr, ",");

    if (cmdToken == nullptr || targetToken == nullptr || parameterToken == nullptr || valueToken == nullptr)
    {
      sendError("BAD_FORMAT");
      clearErrorContext();
      return;
    }

    if (strtok(nullptr, ",") != nullptr)
    {
      sendError("BAD_FORMAT");
      clearErrorContext();
      return;
    }

    Command cmd{};
    cmd.type = parseCommandType(cmdToken);
    cmd.target = parseTargetType(targetToken);
    cmd.parameter = parseParameterType(parameterToken);
    cmd.value = parseValueType(valueToken);
    cmd.rawValueToken = valueToken;
    cmd.numericValue = 0;
    cmd.hasNumericValue = false;

    if (isNumericToken(valueToken))
    {
      cmd.numericValue = strtoul(valueToken, nullptr, 10);
      cmd.hasNumericValue = true;
    }

    executeCommand(cmd);
    clearErrorContext();
  }
}

void readSerialCommands()
{
  while (Serial.available() > 0)
  {
    const char c = static_cast<char>(Serial.read());

    if (c == '\r')
    {
      continue;
    }

    if (c == '\n')
    {
      inputBuffer[inputPos] = '\0';

      if (inputPos > 0)
      {
        processCommand(inputBuffer);
      }

      inputPos = 0;
    }
    else
    {
      if (inputPos < BUFFER_SIZE - 1)
      {
        inputBuffer[inputPos++] = c;
      }
      else
      {
        inputBuffer[inputPos] = '\0';
        setErrorContext(inputBuffer);
        sendError("OVERFLOW");
        clearErrorContext();
        inputPos = 0;
      }
    }
  }
}

void setupProtocol()
{
  inputPos = 0;

  Serial.println("READY");
  Serial.println("Commands:");
  Serial.println("SET,LED,ENABLE,TRUE");
  Serial.println("SET,LED,ENABLE,FALSE");
  Serial.println("SET,LED,TELEMETRY,ENABLE");
  Serial.println("SET,LED,TELEMETRY,DISABLE");
  Serial.println("SET,LED,STATE,ON");
  Serial.println("SET,LED,STATE,OFF");
  Serial.println("SET,LED,COLOR,RED");
  Serial.println("SET,LED,COLOR,GREEN");
  Serial.println("SET,LED,COLOR,BLUE");
  Serial.println("SET,BATTERY,TELEMETRY,ENABLE");
  Serial.println("SET,BATTERY,TELEMETRY,DISABLE");
  Serial.println("SET,GPS,ENABLE,TRUE");
  Serial.println("SET,GPS,ENABLE,FALSE");
  Serial.println("GET,GPS,NONE,NONE");
  Serial.println("SET,TELEMETRY,TELEMETRY,ENABLE");
  Serial.println("SET,TELEMETRY,TELEMETRY,DISABLE");
  Serial.println("SET,RTC,TELEMETRY,ENABLE");
  Serial.println("SET,RTC,TELEMETRY,DISABLE");
  Serial.println("SET,RTC,CURRENT_TIME,2026-03-27T12:00:00Z");
  Serial.println("SET,TELEMETRY,ENABLE,TRUE");
  Serial.println("SET,TELEMETRY,ENABLE,FALSE");
  Serial.println("SET,TELEMETRY,INTERVAL_S,5");
  Serial.println("GET,LED,NONE,NONE");
  Serial.println("GET,TELEMETRY,NONE,NONE");
  Serial.println("GET,BATTERY,NONE,NONE");
  Serial.println("GET,RTC,NONE,NONE");
  Serial.println("GET,STATUS,HEARTBEAT_N,NONE");
  Serial.println("PING,NONE,NONE,NONE");
}
