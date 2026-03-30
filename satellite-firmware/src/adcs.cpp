#include "adcs.h"

#include <Arduino.h>
#include <math.h>

#include "config.h"
#include "imu.h"
#include "sender.h"
#include "telemetry.h"

namespace
{
  struct AdcsState
  {
    bool enabled;
    bool available;
    float rollDeg;
    float pitchDeg;
    float yawRateDps;
  };

  constexpr float kRadiansToDegrees = 57.2957795f;
  AdcsState adcsState{};

  void clearAdcsValues()
  {
    adcsState.rollDeg = 0.0f;
    adcsState.pitchDeg = 0.0f;
    adcsState.yawRateDps = 0.0f;
  }

  void reportAdcsTelemetryStatus()
  {
    sendTelemetry("ADCS", "TELEMETRY", isTargetTelemetryEnabled(TARGET_ADCS) ? "TRUE" : "FALSE");
  }

  void reportAdcsEnableStatus()
  {
    sendTelemetry("ADCS", "ENABLE", adcsState.enabled ? "TRUE" : "FALSE");
  }

  void reportAdcsAvailability()
  {
    sendTelemetry("ADCS", "AVAILABLE", adcsState.available ? "TRUE" : "FALSE");
  }

  void reportAdcsRoll()
  {
    sendTelemetryFloat("ADCS", "ROLL_DEG", adcsState.available ? adcsState.rollDeg : 0.0f, Config::Adcs::attitudeDecimalPlaces);
  }

  void reportAdcsPitch()
  {
    sendTelemetryFloat("ADCS", "PITCH_DEG", adcsState.available ? adcsState.pitchDeg : 0.0f, Config::Adcs::attitudeDecimalPlaces);
  }

  void reportAdcsYawRate()
  {
    sendTelemetryFloat("ADCS", "YAW_RATE_DPS", adcsState.available ? adcsState.yawRateDps : 0.0f, Config::Adcs::yawRateDecimalPlaces);
  }
}

void setupAdcs()
{
  adcsState.enabled = Config::Adcs::defaultEnabled;
  adcsState.available = false;
  clearAdcsValues();
}

void updateAdcs()
{
  if (!adcsState.enabled)
  {
    adcsState.available = false;
    clearAdcsValues();
    return;
  }

  const ImuReadings imuReadings = getImuReadings();
  if (!imuReadings.enabled || !imuReadings.available)
  {
    adcsState.available = false;
    clearAdcsValues();
    return;
  }

  const float x = imuReadings.xMs2;
  const float y = imuReadings.yMs2;
  const float z = imuReadings.zMs2;

  adcsState.rollDeg = atan2f(y, z) * kRadiansToDegrees;
  adcsState.pitchDeg = atan2f(-x, sqrtf((y * y) + (z * z))) * kRadiansToDegrees;
  adcsState.yawRateDps = imuReadings.gyroZDps;
  adcsState.available = true;
}

void reportAdcsStatus()
{
  updateAdcs();

  reportAdcsTelemetryStatus();
  reportAdcsEnableStatus();
  reportAdcsAvailability();
  reportAdcsRoll();
  reportAdcsPitch();
  reportAdcsYawRate();
}

void handleGetAdcs(const Command &cmd)
{
  if (cmd.value != VALUE_NONE)
  {
    sendError("BAD_FORMAT");
    return;
  }

  updateAdcs();

  switch (cmd.parameter)
  {
  case PARAM_NONE:
    reportAdcsStatus();
    break;

  case PARAM_TELEMETRY:
    reportAdcsTelemetryStatus();
    break;

  case PARAM_ENABLE:
    reportAdcsEnableStatus();
    break;

  case PARAM_AVAILABLE:
    reportAdcsAvailability();
    break;

  case PARAM_ROLL_DEG:
    reportAdcsRoll();
    break;

  case PARAM_PITCH_DEG:
    reportAdcsPitch();
    break;

  case PARAM_YAW_RATE_DPS:
    reportAdcsYawRate();
    break;

  default:
    sendError("BAD_PARAMETER");
    break;
  }
}

void handleSetAdcs(const Command &cmd)
{
  switch (cmd.parameter)
  {
  case PARAM_ENABLE:
    if (cmd.value == VALUE_TRUE)
    {
      adcsState.enabled = true;
      sendAck("ADCS", "ENABLE");
    }
    else if (cmd.value == VALUE_FALSE)
    {
      adcsState.enabled = false;
      adcsState.available = false;
      clearAdcsValues();
      sendAck("ADCS", "DISABLE");
    }
    else
    {
      sendError("BAD_VALUE");
    }
    break;

  default:
    sendError("BAD_PARAMETER");
    break;
  }
}
