enum CommandType {
  CMD_UNKNOWN = 0,

  // Implemented now
  CMD_SET,
  CMD_GET,
  CMD_PING,

  // RESERVED FUTURE
  CMD_RESET,
  CMD_SAVE
};

enum TargetType {
  TARGET_UNKNOWN = 0,
  TARGET_NONE,

  // Implemented now
  TARGET_LED,
  TARGET_TELEMETRY,

  // RESERVED FUTURE
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

enum ValueType {
  VALUE_UNKNOWN = 0,

  // Implemented now
  VALUE_ON,
  VALUE_OFF,
  VALUE_NONE,

  // RESERVED FUTUTRE
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

enum ParameterType {
  PARAM_UNKNOWN = 0,

  // General telemetry parameters
  PARAM_STATE,
  PARAM_ENABLE,
  PARAM_MODE,
  PARAM_HEALTH,
  PARAM_INTERVAL_S,
  PARAM_SECONDS
};

struct Command {
  CommandType type;
  TargetType target;
  ValueType value;
};