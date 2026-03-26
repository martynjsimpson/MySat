#ifndef COMMANDS_H
#define COMMANDS_H

#include <Arduino.h>

enum CommandType {
  CMD_UNKNOWN = 0,

  // Implemented now
  CMD_SET,
  CMD_GET,
  CMD_PING,

  // Reserved future
  CMD_RESET,
  CMD_SAVE
};

enum TargetType {
  TARGET_UNKNOWN = 0,
  TARGET_NONE,

  // Implemented now
  TARGET_LED,
  TARGET_TELEMETRY,

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

enum ParameterType {
  PARAM_UNKNOWN = 0,
  PARAM_NONE,

  // General command / telemetry parameters
  PARAM_STATE,
  PARAM_ENABLE,
  PARAM_MODE,
  PARAM_HEALTH,
  PARAM_INTERVAL_S,
  PARAM_SECONDS
};

enum ValueType {
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
  VALUE_FAIL
};

struct Command {
  CommandType type;
  TargetType target;
  ParameterType parameter;
  ValueType value;
  unsigned long numericValue;
  bool hasNumericValue;
};

#endif
