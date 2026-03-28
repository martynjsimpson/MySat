#include <Arduino.h>

#include "rtc.h"
#include "led.h"
#include "pmic.h"
#include "protocol.h"
#include "status.h"
#include "telemetry.h"

void setup()
{
  setupRtc();

  Serial.begin(115200);

  setupLed();
  setupBattery();
  setupStatus();
  setupTelemetry();
  setupProtocol();
}

void loop()
{
  readSerialCommands();
  handlePeriodicTelemetry();
}
