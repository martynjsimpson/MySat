#include "protocol_internal.h"

#include <stdlib.h>
#include <string.h>

#include "sender.h"

namespace
{
  const size_t BUFFER_SIZE = 96;
  char rawCommandBuffer[BUFFER_SIZE];

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
    if (strcmp(token, "IMU") == 0)
      return TARGET_IMU;
    if (strcmp(token, "ADCS") == 0)
      return TARGET_ADCS;
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
    if (strcmp(token, "UPTIME_S") == 0)
      return PARAM_UPTIME_S;
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
    if (strcmp(token, "TEMPERATURE_C") == 0)
      return PARAM_TEMPERATURE_C;
    if (strcmp(token, "HUMIDITY_P") == 0)
      return PARAM_HUMIDITY_P;
    if (strcmp(token, "X_MS2") == 0)
      return PARAM_X_MS2;
    if (strcmp(token, "Y_MS2") == 0)
      return PARAM_Y_MS2;
    if (strcmp(token, "Z_MS2") == 0)
      return PARAM_Z_MS2;
    if (strcmp(token, "GYRO_X_DPS") == 0)
      return PARAM_GYRO_X_DPS;
    if (strcmp(token, "GYRO_Y_DPS") == 0)
      return PARAM_GYRO_Y_DPS;
    if (strcmp(token, "GYRO_Z_DPS") == 0)
      return PARAM_GYRO_Z_DPS;
    if (strcmp(token, "MAG_X_UT") == 0)
      return PARAM_MAG_X_UT;
    if (strcmp(token, "MAG_Y_UT") == 0)
      return PARAM_MAG_Y_UT;
    if (strcmp(token, "MAG_Z_UT") == 0)
      return PARAM_MAG_Z_UT;
    if (strcmp(token, "HEADING_DEG") == 0)
      return PARAM_HEADING_DEG;
    if (strcmp(token, "ROLL_DEG") == 0)
      return PARAM_ROLL_DEG;
    if (strcmp(token, "PITCH_DEG") == 0)
      return PARAM_PITCH_DEG;
    if (strcmp(token, "YAW_RATE_DPS") == 0)
      return PARAM_YAW_RATE_DPS;
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
