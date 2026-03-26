#include "commands.h"
#include "rtc.h"
#include "sender.h"
#include "led.h"
#include "pmic.h"
#include "telemetry.h"

const size_t BUFFER_SIZE = 96;
char inputBuffer[BUFFER_SIZE];
size_t inputPos = 0;

void readSerialCommands();
void processCommand(char *line);
bool isNumericToken(const char *token);
CommandType parseCommandType(const char *token);
TargetType parseTargetType(const char *token);
ParameterType parseParameterType(const char *token);
ValueType parseValueType(const char *token);
void executeCommand(const Command &cmd);
void handleSet(const Command &cmd);
void handleGet(const Command &cmd);
void handleSetTelemetry(const Command &cmd);
void handlePing(const Command &cmd);
void sendTelemetrySnapshot();
void handlePeriodicTelemetry();
void reportTelemetryStatus();

void setup() {
  setupRtc();
  setupLed();
  setupBattery();

  Serial.begin(115200);
  //while (!Serial) {
  //  ;
  //}

  lastTelemetryTime = getTimestamp();

  Serial.println("READY");
  Serial.println("Commands:");
  Serial.println("SET,LED,ENABLE,TRUE");
  Serial.println("SET,LED,ENABLE,FALSE");
  Serial.println("SET,LED,STATE,ON");
  Serial.println("SET,LED,STATE,OFF");
  Serial.println("SET,TELEMETRY,ENABLE,TRUE");
  Serial.println("SET,TELEMETRY,ENABLE,FALSE");
  Serial.println("SET,TELEMETRY,INTERVAL_S,5");
  Serial.println("GET,LED,NONE,NONE");
  Serial.println("GET,TELEMETRY,NONE,NONE");
  Serial.println("PING,NONE,NONE,NONE");
}

void loop() {
  readSerialCommands();
  handlePeriodicTelemetry();
}

void readSerialCommands() {
  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      inputBuffer[inputPos] = '\0';

      if (inputPos > 0) {
        processCommand(inputBuffer);
      }

      inputPos = 0;
    } else {
      if (inputPos < BUFFER_SIZE - 1) {
        inputBuffer[inputPos++] = c;
      } else {
        sendError("OVERFLOW");
        inputPos = 0;
      }
    }
  }
}

void processCommand(char *line) {
  char *cmdToken = strtok(line, ",");
  char *targetToken = strtok(NULL, ",");
  char *parameterToken = strtok(NULL, ",");
  char *valueToken = strtok(NULL, ",");

  if (cmdToken == NULL || targetToken == NULL || parameterToken == NULL || valueToken == NULL) {
    sendError("BAD_FORMAT");
    return;
  }

  if (strtok(NULL, ",") != NULL) {
    sendError("BAD_FORMAT");
    return;
  }

  Command cmd;
  cmd.type = parseCommandType(cmdToken);
  cmd.target = parseTargetType(targetToken);
  cmd.parameter = parseParameterType(parameterToken);
  cmd.value = parseValueType(valueToken);
  cmd.numericValue = 0;
  cmd.hasNumericValue = false;

  if (isNumericToken(valueToken)) {
    cmd.numericValue = strtoul(valueToken, NULL, 10);
    cmd.hasNumericValue = true;
  }

  executeCommand(cmd);
}

bool isNumericToken(const char *token) {
  if (token == NULL || *token == '\0') {
    return false;
  }

  for (const char *p = token; *p != '\0'; p++) {
    if (*p < '0' || *p > '9') {
      return false;
    }
  }

  return true;
}

CommandType parseCommandType(const char *token) {
  if (strcmp(token, "SET") == 0) return CMD_SET;
  if (strcmp(token, "GET") == 0) return CMD_GET;
  if (strcmp(token, "PING") == 0) return CMD_PING;
  if (strcmp(token, "RESET") == 0) return CMD_RESET;
  if (strcmp(token, "SAVE") == 0) return CMD_SAVE;
  return CMD_UNKNOWN;
}

TargetType parseTargetType(const char *token) {
  if (strcmp(token, "NONE") == 0) return TARGET_NONE;
  if (strcmp(token, "LED") == 0) return TARGET_LED;
  if (strcmp(token, "TELEMETRY") == 0) return TARGET_TELEMETRY;
  if (strcmp(token, "MODE") == 0) return TARGET_MODE;
  if (strcmp(token, "STATUS") == 0) return TARGET_STATUS;
  if (strcmp(token, "RADIO") == 0) return TARGET_RADIO;
  if (strcmp(token, "POWER") == 0) return TARGET_POWER;
  if (strcmp(token, "PAYLOAD") == 0) return TARGET_PAYLOAD;
  if (strcmp(token, "THERMAL") == 0) return TARGET_THERMAL;
  if (strcmp(token, "LOG") == 0) return TARGET_LOG;
  if (strcmp(token, "WATCHDOG") == 0) return TARGET_WATCHDOG;
  if (strcmp(token, "UPTIME") == 0) return TARGET_UPTIME;
  return TARGET_UNKNOWN;
}

ParameterType parseParameterType(const char *token) {
  if (strcmp(token, "NONE") == 0) return PARAM_NONE;
  if (strcmp(token, "STATE") == 0) return PARAM_STATE;
  if (strcmp(token, "ENABLE") == 0) return PARAM_ENABLE;
  if (strcmp(token, "MODE") == 0) return PARAM_MODE;
  if (strcmp(token, "HEALTH") == 0) return PARAM_HEALTH;
  if (strcmp(token, "INTERVAL_S") == 0) return PARAM_INTERVAL_S;
  if (strcmp(token, "SECONDS") == 0) return PARAM_SECONDS;
  return PARAM_UNKNOWN;
}

ValueType parseValueType(const char *token) {
  if (strcmp(token, "ON") == 0) return VALUE_ON;
  if (strcmp(token, "OFF") == 0) return VALUE_OFF;
  if (strcmp(token, "NONE") == 0) return VALUE_NONE;
  if (strcmp(token, "TRUE") == 0) return VALUE_TRUE;
  if (strcmp(token, "FALSE") == 0) return VALUE_FALSE;
  if (strcmp(token, "ENABLE") == 0) return VALUE_ENABLE;
  if (strcmp(token, "DISABLE") == 0) return VALUE_DISABLE;
  if (strcmp(token, "SAFE") == 0) return VALUE_SAFE;
  if (strcmp(token, "NORMAL") == 0) return VALUE_NORMAL;
  if (strcmp(token, "LOW_POWER") == 0) return VALUE_LOW_POWER;
  if (strcmp(token, "ACTIVE") == 0) return VALUE_ACTIVE;
  if (strcmp(token, "IDLE") == 0) return VALUE_IDLE;
  if (strcmp(token, "OK") == 0) return VALUE_OK;
  if (strcmp(token, "FAIL") == 0) return VALUE_FAIL;
  return VALUE_UNKNOWN;
}

void executeCommand(const Command &cmd) {
  switch (cmd.type) {
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

void handleSet(const Command &cmd) {
  switch (cmd.target) {
    case TARGET_LED:
      handleSetLed(cmd);
      break;

    case TARGET_TELEMETRY:
      handleSetTelemetry(cmd);
      break;

    default:
      sendError("UNKNOWN_TARGET");
      break;
  }
}

void handleGet(const Command &cmd) {
  if (cmd.parameter != PARAM_NONE || cmd.value != VALUE_NONE) {
    sendError("BAD_FORMAT");
    return;
  }

  switch (cmd.target) {
    case TARGET_LED:
      reportLedStatus();
      break;

    case TARGET_TELEMETRY:
      reportTelemetryStatus();
      break;

    case TARGET_BATTERY:
      //
      break;
    default:
      sendError("UNKNOWN_TARGET");
      break;
  }
}

void handlePing(const Command &cmd) {
  if (cmd.target == TARGET_NONE &&
      cmd.parameter == PARAM_NONE &&
      cmd.value == VALUE_NONE) {
    sendAck("PING", "PONG");
  } else {
    sendError("BAD_FORMAT");
  }
}
