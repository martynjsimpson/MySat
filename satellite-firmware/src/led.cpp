#include "led.h"

#include <Arduino.h>
#include <string.h>

#include "config.h"
#include "sender.h"
#include "telemetry.h"

namespace
{
  const int LED_PIN = LED_BUILTIN;

  bool ledEnabled = Config::Led::defaultEnabled;
  bool ledStateOn = Config::Led::defaultStateOn;

  enum LedColor
  {
    LED_COLOR_RED = 0,
    LED_COLOR_GREEN,
    LED_COLOR_BLUE
  };

  LedColor defaultLedColor()
  {
    if (strcmp(Config::Led::defaultColor, "RED") == 0)
    {
      return LED_COLOR_RED;
    }
    if (strcmp(Config::Led::defaultColor, "BLUE") == 0)
    {
      return LED_COLOR_BLUE;
    }
    return LED_COLOR_GREEN;
  }

  LedColor ledColor = defaultLedColor();

  void setLed(bool on)
  {
    digitalWrite(LED_PIN, on ? HIGH : LOW);
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

  void reportLedTelemetryStatus()
  {
    sendTelemetry("LED", "TELEMETRY", isTargetTelemetryEnabled(TARGET_LED) ? "TRUE" : "FALSE");
  }

  void reportLedEnableStatus()
  {
    sendTelemetry("LED", "ENABLE", ledEnabled ? "TRUE" : "FALSE");
  }

  void reportLedStateStatus()
  {
    sendTelemetry("LED", "STATE", ledStateOn ? "ON" : "OFF");
  }

  void reportLedColorStatus()
  {
    sendTelemetry("LED", "COLOR", colorToToken(ledColor));
  }

  void applyLedOutput()
  {
    if (!ledEnabled || !ledStateOn)
    {
      setLed(false);
      return;
    }

    // The MKR WAN 1310 only guarantees the built-in status LED. We retain the
    // protocol-level color token for compatibility, but hardware output is
    // reduced to simple on/off behavior.
    setLed(true);
  }
} // namespace

void setupLed()
{
  pinMode(LED_PIN, OUTPUT);

  ledEnabled = Config::Led::defaultEnabled;
  ledStateOn = Config::Led::defaultStateOn;
  ledColor = defaultLedColor();
  applyLedOutput();
}

void handleGetLed(const Command &cmd)
{
  if (cmd.value != VALUE_NONE)
  {
    sendError("BAD_FORMAT");
    return;
  }

  switch (cmd.parameter)
  {
  case PARAM_NONE:
    reportLedStatus();
    break;

  case PARAM_TELEMETRY:
    reportLedTelemetryStatus();
    break;

  case PARAM_ENABLE:
    reportLedEnableStatus();
    break;

  case PARAM_STATE:
    reportLedStateStatus();
    break;

  case PARAM_COLOR:
    reportLedColorStatus();
    break;

  default:
    sendError("BAD_PARAMETER");
    break;
  }
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
  reportLedTelemetryStatus();
  reportLedEnableStatus();
  reportLedStateStatus();
  reportLedColorStatus();
}
