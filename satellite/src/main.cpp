#include <Arduino.h>

#include "gps.h"
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
  setupGps();
  setupStatus();
  setupTelemetry();
  setupProtocol();
  delay(6000);
  reportStatusStarted();
  reportStatusHeartbeat(false);
  resetTelemetrySchedule();
}

void loop()
{
  updateGps();
  handleRtcAutoSync();
  readSerialCommands();
  handlePeriodicTelemetry();
}
