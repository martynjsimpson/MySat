#ifndef COMMANDS_H
#define COMMANDS_H

#include <Arduino.h>

enum CommandType
{
  CMD_UNKNOWN = 0,

  // Implemented now
  CMD_SET,
  CMD_GET,
  CMD_PING,

  // Reserved future
  CMD_RESET,
  CMD_SAVE
};

enum TargetType
{
  TARGET_UNKNOWN = 0,
  TARGET_NONE,

  // Implemented now
  TARGET_LED,
  TARGET_TELEMETRY,
  TARGET_BATTERY,
  TARGET_GPS,
  TARGET_RTC,

  // Reserved future
  TARGET_MODE,
  TARGET_STATUS,
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

  // General command / telemetry parameters
  PARAM_STATE,
  PARAM_ENABLE,
  PARAM_MODE,
  PARAM_HEALTH,
  PARAM_INTERVAL_S,
  PARAM_TELEMETRY,
  PARAM_CURRENT_TIME,
  PARAM_HEARTBEAT_N,
  PARAM_SYNC,
  PARAM_SECONDS,
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
  PARAM_SATELLITES_N
};

enum ValueType
{
  VALUE_UNKNOWN = 0,

  // Implemented now
  VALUE_ON,
  VALUE_OFF,
  VALUE_NONE,
  VALUE_TRUE,
  VALUE_FALSE,

  // Reserved future
  VALUE_ENABLE,
  VALUE_DISABLE,
  VALUE_SAFE,
  VALUE_NORMAL,
  VALUE_LOW_POWER,
  VALUE_ACTIVE,
  VALUE_IDLE,

  // Generic status values
  VALUE_OK,
  VALUE_FAIL,
  VALUE_RED,
  VALUE_GREEN,
  VALUE_BLUE
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
