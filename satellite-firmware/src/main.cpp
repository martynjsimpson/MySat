#include <Arduino.h>

#include "config.h"
#include "gps.h"
#include "led.h"
#include "pmic.h"
#include "protocol.h"
#include "rtc.h"
#include "status.h"
#include "telemetry.h"
#include "transport.h"

void setup()
{
  setupRtc();
  setupTransport();
  setupLed();
  setupBattery();
  setupGps();
  setupStatus();
  setupTelemetry();
  setupProtocol();
  delay(Config::Main::serialStartupDelayMs);
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
