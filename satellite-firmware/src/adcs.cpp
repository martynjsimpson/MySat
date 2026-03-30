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
    const char *source;
    float rollDeg;
    float pitchDeg;
    float yawRateDps;
    float headingDeg;
  };

  constexpr float kRadiansToDegrees = 57.2957795f;
  constexpr float kDegreesToRadians = 0.0174532925f;
  AdcsState adcsState{};

  void clearAdcsValues()
  {
    adcsState.rollDeg = 0.0f;
    adcsState.pitchDeg = 0.0f;
    adcsState.yawRateDps = 0.0f;
    adcsState.headingDeg = 0.0f;
  }

  float axisByIndex(const ImuReadings &imuReadings, uint8_t axis)
  {
    switch (axis)
    {
    case 0:
      return imuReadings.magXUt;
    case 1:
      return imuReadings.magYUt;
    case 2:
      return imuReadings.magZUt;
    default:
      return 0.0f;
    }
  }

  float normalizeHeadingDeg(float headingDeg)
  {
    while (headingDeg < 0.0f)
    {
      headingDeg += 360.0f;
    }

    while (headingDeg >= 360.0f)
    {
      headingDeg -= 360.0f;
    }

    return headingDeg;
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

  void reportAdcsSource()
  {
    sendTelemetry("ADCS", "SOURCE", adcsState.source);
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

  void reportAdcsHeading()
  {
    sendTelemetryFloat("ADCS", "HEADING_DEG", adcsState.available ? adcsState.headingDeg : 0.0f, Config::Adcs::headingDecimalPlaces);
  }
}

void setupAdcs()
{
  adcsState.enabled = Config::Adcs::defaultEnabled;
  adcsState.available = false;
  adcsState.source = "NONE";
  clearAdcsValues();
}

void updateAdcs()
{
  if (!adcsState.enabled)
  {
    adcsState.available = false;
    adcsState.source = "NONE";
    clearAdcsValues();
    return;
  }

  const ImuReadings imuReadings = getImuReadings();
  if (!imuReadings.enabled || !imuReadings.available)
  {
    adcsState.available = false;
    adcsState.source = "NONE";
    clearAdcsValues();
    return;
  }

  adcsState.source = imuReadings.magnetometerAvailable ? "ACCEL_GYRO_MAG" : "ACCEL_GYRO";

  if (!imuReadings.magnetometerAvailable)
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

  const float magBodyX = static_cast<float>(Config::Adcs::magBodyXSign) *
                         axisByIndex(imuReadings, Config::Adcs::magBodyXSourceAxis);
  const float magBodyY = static_cast<float>(Config::Adcs::magBodyYSign) *
                         axisByIndex(imuReadings, Config::Adcs::magBodyYSourceAxis);
  const float magBodyZ = static_cast<float>(Config::Adcs::magBodyZSign) *
                         axisByIndex(imuReadings, Config::Adcs::magBodyZSourceAxis);

  const float rollRad = adcsState.rollDeg * kDegreesToRadians;
  const float pitchRad = adcsState.pitchDeg * kDegreesToRadians;
  const float cosRoll = cosf(rollRad);
  const float sinRoll = sinf(rollRad);
  const float cosPitch = cosf(pitchRad);
  const float sinPitch = sinf(pitchRad);

  const float magHorizontalX = (magBodyX * cosPitch) + (magBodyZ * sinPitch);
  const float magHorizontalY = (magBodyX * sinRoll * sinPitch) +
                               (magBodyY * cosRoll) -
                               (magBodyZ * sinRoll * cosPitch);

  adcsState.headingDeg = normalizeHeadingDeg(atan2f(magHorizontalY, magHorizontalX) * kRadiansToDegrees);
  adcsState.available = true;
}

void reportAdcsStatus()
{
  updateAdcs();

  reportAdcsTelemetryStatus();
  reportAdcsEnableStatus();
  reportAdcsAvailability();
  reportAdcsSource();
  reportAdcsRoll();
  reportAdcsPitch();
  reportAdcsYawRate();
  reportAdcsHeading();
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

  case PARAM_SOURCE:
    reportAdcsSource();
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

  case PARAM_HEADING_DEG:
    reportAdcsHeading();
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
      adcsState.source = "NONE";
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
