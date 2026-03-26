#include "commands.h"
#include "rtc.h"
#include "sender.h"
#include "led.h"

const size_t BUFFER_SIZE = 64;
char inputBuffer[BUFFER_SIZE];
size_t inputPos = 0;

bool telemetryEnabled = true;
const unsigned long telemetryIntervalSeconds = 5;
unsigned long lastTelemetryTime = 0;

void setup() {
  setupRtc();
  setupLed();

  Serial.begin(115200);
  while (!Serial) {
    ;
  }

  Serial.println("READY");
  Serial.println("Commands:");
  Serial.println("SET,LED,ENABLE");
  Serial.println("SET,LED,DISABLE");
  Serial.println("SET,LED,ON");
  Serial.println("SET,LED,OFF");
  Serial.println("GET,LED,NONE");
  Serial.println("PING,NONE,NONE");
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
        Serial.println("ERR,OVERFLOW");
        inputPos = 0;
      }
    }
  }
}

void processCommand(char *line) {
  char *cmdToken    = strtok(line, ",");
  char *targetToken = strtok(NULL, ",");
  char *valueToken  = strtok(NULL, ",");

  if (cmdToken == NULL || targetToken == NULL || valueToken == NULL) {
    sendError("BAD_FORMAT");
    return;
  }

  Command cmd;
  cmd.type = parseCommandType(cmdToken);
  cmd.target = parseTargetType(targetToken);
  cmd.value = parseValueType(valueToken);

  executeCommand(cmd);
}

CommandType parseCommandType(const char *token) {
  if (strcmp(token, "SET") == 0)  return CMD_SET;
  if (strcmp(token, "GET") == 0)  return CMD_GET;
  if (strcmp(token, "PING") == 0) return CMD_PING;
  if (strcmp(token, "RESET") == 0) return CMD_RESET;
  if (strcmp(token, "SAVE") == 0) return CMD_SAVE;
  return CMD_UNKNOWN;
}

TargetType parseTargetType(const char *token) {
  if (strcmp(token, "NONE") == 0) return TARGET_NONE;
  if (strcmp(token, "LED") == 0) return TARGET_LED;
  if (strcmp(token, "MODE") == 0) return TARGET_MODE;
  if (strcmp(token, "STATUS") == 0) return TARGET_STATUS;
  if (strcmp(token, "RADIO") == 0) return TARGET_RADIO; 
  if (strcmp(token, "POWER") == 0) return TARGET_POWER;
  if (strcmp(token, "PAYLOAD") == 0) return TARGET_PAYLOAD;
  if (strcmp(token, "THERMAL") == 0) return TARGET_THERMAL;
  if (strcmp(token, "LOG") == 0) return TARGET_LOG;
  if (strcmp(token, "WATCHDOG") == 0) return TARGET_WATCHDOG;
  if (strcmp(token, "UPTIME") == 0) return TARGET_UPTIME;
  if (strcmp(token, "TELEMETRY") == 0) return TARGET_TELEMETRY;
  return TARGET_UNKNOWN;
}

ValueType parseValueType(const char *token) {
  if (strcmp(token, "ON") == 0) return VALUE_ON;
  if (strcmp(token, "OFF") == 0) return VALUE_OFF;
  if (strcmp(token, "NONE") == 0) return VALUE_NONE;
  if (strcmp(token, "ENABLED") == 0) return VALUE_ENABLE;
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
      handleSetLed(cmd.value);
      break;

    case TARGET_TELEMETRY:
      handleSetTelemetry(cmd.value);
      break;

    default:
      sendError("UNKNOWN_TARGET");
      break;
  }
}

void handleGet(const Command &cmd) {
  switch (cmd.target) {
    case TARGET_LED:
      if (cmd.value == VALUE_NONE) {
        reportLedStatus();
      } else {
        sendError("BAD_VALUE");
      }
      break;
    
    case TARGET_TELEMETRY:
      if (cmd.value == VALUE_NONE) {
        reportTelemetryStatus();
      } else {
        sendError("BAD_VALUE");
      }
      break;

    default:
      sendError("UNKNOWN_TARGET");
      break;
  }
}

void handleSetTelemetry(ValueType value) {
  switch (value) {
    case VALUE_ENABLE:
      telemetryEnabled = true;
      sendAck("TELEMETRY", "ENABLE");
      break;

    case VALUE_DISABLE:
      telemetryEnabled = false;
      sendAck("TELEMETRY", "DISABLE");
      break;

    default:
      sendError("BAD_VALUE");
      break;
  }
}

void handlePing(const Command &cmd) {
  if (cmd.target == TARGET_NONE && cmd.value == VALUE_NONE) {
    sendAck("PONG","0");
  } else {
    sendError("BAD_FORMAT");
  }
}

void sendTelemetrySnapshot() {
  reportLedStatus();
  reportTelemetryStatus();
}

void handlePeriodicTelemetry() {
  if (!telemetryEnabled) {
    return;
  }

  unsigned long now = getTimestamp();

  if ((now - lastTelemetryTime) >= telemetryIntervalSeconds) {
    lastTelemetryTime = now;
    sendTelemetrySnapshot();
  }
}

void reportTelemetryStatus() {
  sendTelemetry("TELEMETRY", "ENABLE", telemetryEnabled ? "TRUE" : "FALSE");
  sendTelemetryULong("TELEMETRY", "INTERVAL_S", telemetryIntervalSeconds);
}

void sendTelemetryULong(const char *target, const char *parameter, unsigned long value) {
  Serial.print(getTimestamp());
  Serial.print(",TLM,");
  Serial.print(target);
  Serial.print(",");
  Serial.print(parameter);
  Serial.print(",");
  Serial.println(value);
}

