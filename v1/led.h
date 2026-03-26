#ifndef LED_HELPER_H
#define LED_HELPER_H

#include <Arduino.h>

const int LED_PIN = LED_BUILTIN;

// Control policy state
bool ledEnabled = false;

void setupLed() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void setLed(bool on) {
  digitalWrite(LED_PIN, on ? HIGH : LOW);
}

void handleSetLed(const Command &cmd) {
  switch (cmd.parameter) {
    case PARAM_ENABLE:
      if (cmd.value == VALUE_TRUE) {
        ledEnabled = true;
        sendAck("LED", "ENABLE");
      } else if (cmd.value == VALUE_FALSE) {
        ledEnabled = false;
        setLed(false);
        sendAck("LED", "DISABLE");
      } else {
        sendError("BAD_VALUE");
      }
      break;

    case PARAM_STATE:
      if (cmd.value == VALUE_ON) {
        if (!ledEnabled) {
          sendError("LED_DISABLED");
          return;
        }
        setLed(true);
        sendAck("LED", "ON");
      } else if (cmd.value == VALUE_OFF) {
        setLed(false);
        sendAck("LED", "OFF");
      } else {
        sendError("BAD_VALUE");
      }
      break;

    default:
      sendError("BAD_PARAMETER");
      break;
  }
}

void reportLedStatus() {
  int pinState = digitalRead(LED_PIN);

  sendTelemetry("LED", "ENABLE", ledEnabled ? "TRUE" : "FALSE");
  sendTelemetry("LED", "STATE", pinState == HIGH ? "ON" : "OFF");
}

#endif
