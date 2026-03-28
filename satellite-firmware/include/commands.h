#ifndef COMMANDS_H
#define COMMANDS_H

#include <Arduino.h>

enum CommandType
{
  CMD_UNKNOWN = 0,

  // Implemented
  CMD_SET,
  CMD_GET,
  CMD_PING,
  CMD_RESET,

  // Reserved
  CMD_SAVE
};

enum TargetType
{
  TARGET_UNKNOWN = 0,
  TARGET_NONE,

  // Implemented
  TARGET_LED,
  TARGET_TELEMETRY,
  TARGET_BATTERY,
  TARGET_GPS,
  TARGET_RTC,
  TARGET_STATUS,

  // Reserved
  TARGET_MODE,
  TARGET_RADIO,
  TARGET_POWER,
  TARGET_PAYLOAD,
  TARGET_THERMAL,
  TARGET_LOG,
  TARGET_WATCHDOG,
  TARGET_UPTIME
};

enum ParameterType
{
  PARAM_UNKNOWN = 0,
  PARAM_NONE,

  // Implemented
  PARAM_STATE,
  PARAM_ENABLE,
  PARAM_INTERVAL_S,
  PARAM_TELEMETRY,
  PARAM_CURRENT_TIME,
  PARAM_HEARTBEAT_N,
  PARAM_SYNC,
  PARAM_COLOR,
  PARAM_AVAILABLE,
  PARAM_CHARGE_CURRENT_A,
  PARAM_CHARGE_VOLTAGE_V,
  PARAM_CHARGE_PERCENT_P,
  PARAM_VOLTAGE_V,
  PARAM_LATITUDE_D,
  PARAM_LONGITUDE_D,
  PARAM_ALTITUDE_M,
  PARAM_SPEED_KPH,
  PARAM_SATELLITES_N,

  // Reserved
  PARAM_MODE,
  PARAM_HEALTH,
  PARAM_UPTIME_S
};

enum ValueType
{
  VALUE_UNKNOWN = 0,

  // Implemented
  VALUE_ON,
  VALUE_OFF,
  VALUE_NONE,
  VALUE_TRUE,
  VALUE_FALSE,
  VALUE_ENABLE,
  VALUE_DISABLE,
  VALUE_RED,
  VALUE_GREEN,
  VALUE_BLUE,

  // Reserved
  VALUE_SAFE,
  VALUE_NORMAL,
  VALUE_LOW_POWER,
  VALUE_ACTIVE,
  VALUE_IDLE,
  VALUE_OK,
  VALUE_FAIL
};

struct Command
{
  CommandType type;
  TargetType target;
  ParameterType parameter;
  ValueType value;
  const char *rawValueToken;
  unsigned long numericValue;
  bool hasNumericValue;
};

#endif
