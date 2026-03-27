#include "led.h"

#include <Arduino.h>

#include <WiFiNINA.h>
#include <utility/wifi_drv.h>

#include "sender.h"

namespace
{
  const int LED_PIN = LED_BUILTIN;
  // Some MKR WiFi 1010 revisions have red/green swapped compared to older docs.
  const int RGB_RED_PIN = 25;
  const int RGB_GREEN_PIN = 26;
  const int RGB_BLUE_PIN = 27;

  bool ledEnabled = false;
  bool ledStateOn = false;

  enum LedColor
  {
    LED_COLOR_RED = 0,
    LED_COLOR_GREEN,
    LED_COLOR_BLUE
  };

  LedColor ledColor = LED_COLOR_GREEN;

  void setLed(bool on)
  {
    digitalWrite(LED_PIN, on ? HIGH : LOW);
  }

  void setRgb(bool redOn, bool greenOn, bool blueOn)
  {
    WiFiDrv::digitalWrite(RGB_RED_PIN, redOn ? HIGH : LOW);
    WiFiDrv::digitalWrite(RGB_GREEN_PIN, greenOn ? HIGH : LOW);
    WiFiDrv::digitalWrite(RGB_BLUE_PIN, blueOn ? HIGH : LOW);
  }

  const char *colorToToken(LedColor color)
  {
    switch (color)
    {
    case LED_COLOR_RED:
      return "RED";
    case LED_COLOR_GREEN:
      return "GREEN";
    case LED_COLOR_BLUE:
      return "BLUE";
    default:
      return "UNKNOWN";
    }
  }

  void applyLedOutput()
  {
    if (!ledEnabled || !ledStateOn)
    {
      setLed(false);
      setRgb(false, false, false);
      return;
    }

    setLed(true);
    switch (ledColor)
    {
    case LED_COLOR_RED:
      setRgb(true, false, false);
      break;
    case LED_COLOR_GREEN:
      setRgb(false, true, false);
      break;
    case LED_COLOR_BLUE:
      setRgb(false, false, true);
      break;
    default:
      setRgb(false, false, false);
      break;
    }
  }
} // namespace

void setupLed()
{
  pinMode(LED_PIN, OUTPUT);
  WiFiDrv::pinMode(RGB_RED_PIN, OUTPUT);   // define RED LED
  WiFiDrv::pinMode(RGB_GREEN_PIN, OUTPUT); // define GREEN LED
  WiFiDrv::pinMode(RGB_BLUE_PIN, OUTPUT);  // define BLUE LED

  ledEnabled = false;
  ledStateOn = false;
  ledColor = LED_COLOR_GREEN;
  applyLedOutput();
}

void handleSetLed(const Command &cmd)
{
  switch (cmd.parameter)
  {
  case PARAM_ENABLE:
    if (cmd.value == VALUE_TRUE)
    {
      ledEnabled = true;
      applyLedOutput();
      sendAck("LED", "ENABLE");
    }
    else if (cmd.value == VALUE_FALSE)
    {
      ledEnabled = false;
      ledStateOn = false; // disabling LED also forces it off
      applyLedOutput();
      sendAck("LED", "DISABLE");
    }
    else
    {
      sendError("BAD_VALUE");
    }
    break;

  case PARAM_STATE:
    if (cmd.value == VALUE_ON)
    {
      if (!ledEnabled)
      {
        sendError("LED_DISABLED");
        return;
      }

      ledStateOn = true;
      applyLedOutput();
      sendAck("LED", "ON");
    }
    else if (cmd.value == VALUE_OFF)
    {
      ledStateOn = false;
      applyLedOutput();
      sendAck("LED", "OFF");
    }
    else
    {
      sendError("BAD_VALUE");
    }
    break;

  case PARAM_COLOR:
    if (cmd.value == VALUE_RED)
    {
      ledColor = LED_COLOR_RED;
    }
    else if (cmd.value == VALUE_GREEN)
    {
      ledColor = LED_COLOR_GREEN;
    }
    else if (cmd.value == VALUE_BLUE)
    {
      ledColor = LED_COLOR_BLUE;
    }
    else
    {
      sendError("BAD_VALUE");
      return;
    }

    applyLedOutput();
    sendAck("LED", colorToToken(ledColor));
    break;

  default:
    sendError("BAD_PARAMETER");
    break;
  }
}

void reportLedStatus()
{
  sendTelemetry("LED", "ENABLE", ledEnabled ? "TRUE" : "FALSE");
  sendTelemetry("LED", "STATE", ledStateOn ? "ON" : "OFF");
  sendTelemetry("LED", "COLOR", colorToToken(ledColor));
}
