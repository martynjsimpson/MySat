#include <Arduino.h>

#include "adcs.h"
#include "imu.h"
#include "config.h"
#include "gps.h"
#include "led.h"
#include "pmic.h"
#include "protocol.h"
#include "rtc.h"
#include "status.h"
#include "thermal.h"
#include "telemetry.h"
#include "transport.h"

void setup()
{
  setupRtc();
  setupTransport();
  setupLed();
  setupBattery();
  setupGps();
  setupThermal();
  setupImu();
  setupAdcs();
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
  updateThermal();
  updateImu();
  updateAdcs();
  updateLed();
  handleRtcAutoSync();
  readIncomingCommands();
  handlePeriodicTelemetry();
}
