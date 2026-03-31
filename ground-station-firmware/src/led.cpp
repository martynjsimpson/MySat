#include "led.h"

#include <Arduino.h>

#include "config.h"

namespace
{
  constexpr int kLedPin = LED_BUILTIN;

  unsigned long ledPulseStartMs = 0;
  bool ledPulseActive = false;

  void applyLedOutput()
  {
    digitalWrite(kLedPin, ledPulseActive ? HIGH : LOW);
  }
}

void setupLed()
{
  pinMode(kLedPin, OUTPUT);
  ledPulseActive = false;
  ledPulseStartMs = 0;
  applyLedOutput();
}

void noteLedActivity()
{
  ledPulseActive = true;
  ledPulseStartMs = millis();
  applyLedOutput();
}

void updateLed()
{
  if (!ledPulseActive)
  {
    return;
  }

  if ((millis() - ledPulseStartMs) < Config::Led::activityPulseMs)
  {
    return;
  }

  ledPulseActive = false;
  applyLedOutput();
}
