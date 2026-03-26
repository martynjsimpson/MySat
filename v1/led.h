const int LED_PIN = LED_BUILTIN;

// initial states
bool ledEnabled = false;

void setupLed() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void setLed(bool on) {
  digitalWrite(LED_PIN, on ? HIGH : LOW);
}

void handleSetLed(ValueType value) {
  switch (value) {
    case VALUE_ENABLE:
      ledEnabled = true;
      sendAck("LED","ENABLE");
      break;

    case VALUE_DISABLE:
      ledEnabled = false;
      setLed(false);
      sendAck("LED","DISABLE");
      break;

    case VALUE_ON:
      if (!ledEnabled) {
        sendError("LED_DISABLED");
        return;
      }
      setLed(true);
      sendAck("LED","ON");
      break;

    case VALUE_OFF:
      setLed(false);
      sendAck("LED","OFF");
      break;

    default:
      sendError("BAD_VALUE");
      break;
  }
}



void reportLedStatus() {
  int pinState = digitalRead(LED_PIN);

  sendTelemetry("LED", "ENABLE", ledEnabled ? "TRUE" : "FALSE");
  sendTelemetry("LED", "STATE", pinState == HIGH ? "ON" : "OFF");
}