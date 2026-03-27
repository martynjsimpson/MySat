#include <Arduino.h>

#include "rtc.h"
#include "led.h"
#include "pmic.h"
#include "protocol.h"
#include "telemetry.h"

void setup()
{
  setupRtc();

  Serial.begin(115200);

  setupLed();
  setupBattery();
  setupTelemetry();
  setupProtocol();
}

void loop()
{
  readSerialCommands();
  handlePeriodicTelemetry();
}
