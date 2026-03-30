#include "imu.h"

#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "sender.h"
#include "telemetry.h"

namespace
{
  constexpr uint8_t kPowerManagementRegister = 0x6B;
  constexpr uint8_t kWhoAmIRegister = 0x75;
  constexpr uint8_t kImuDataRegister = 0x3B;
  constexpr uint8_t kExpectedWhoAmI = 0x68;
  constexpr uint8_t kAlternateWhoAmI = 0x69;
  constexpr float kStandardGravityMs2 = 9.80665f;
  constexpr float kImuScaleLsbPerG = 16384.0f;
  constexpr float kGyroScaleLsbPerDps = 131.0f;

  struct ImuState
  {
    bool enabled;
    bool initialized;
    bool available;
    float xMs2;
    float yMs2;
    float zMs2;
    float gyroXDps;
    float gyroYDps;
    float gyroZDps;
    unsigned long lastReadMs;
    bool hasAttemptedRead;
  };

  ImuState imuState{};

  bool writeRegister(uint8_t reg, uint8_t value)
  {
    Wire.beginTransmission(Config::Imu::i2cAddress);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
  }

  bool readRegisters(uint8_t startRegister, uint8_t *buffer, size_t length)
  {
    Wire.beginTransmission(Config::Imu::i2cAddress);
    Wire.write(startRegister);
    if (Wire.endTransmission(false) != 0)
    {
      return false;
    }

    const size_t bytesRead = Wire.requestFrom(static_cast<int>(Config::Imu::i2cAddress), static_cast<int>(length));
    if (bytesRead != length)
    {
      return false;
    }

    for (size_t i = 0; i < length; ++i)
    {
      if (!Wire.available())
      {
        return false;
      }

      buffer[i] = static_cast<uint8_t>(Wire.read());
    }

    return true;
  }

  void clearImuValues()
  {
    imuState.xMs2 = 0.0f;
    imuState.yMs2 = 0.0f;
    imuState.zMs2 = 0.0f;
    imuState.gyroXDps = 0.0f;
    imuState.gyroYDps = 0.0f;
    imuState.gyroZDps = 0.0f;
  }

  bool canReadNow(unsigned long nowMs)
  {
    return !imuState.hasAttemptedRead ||
           (nowMs - imuState.lastReadMs) >= Config::Imu::minReadIntervalMs;
  }

  bool initializeSensor()
  {
    uint8_t whoAmI = 0;
    if (!readRegisters(kWhoAmIRegister, &whoAmI, 1))
    {
      return false;
    }

    if (whoAmI != kExpectedWhoAmI && whoAmI != kAlternateWhoAmI)
    {
      return false;
    }

    return writeRegister(kPowerManagementRegister, 0x00);
  }

  void reportImuTelemetryStatus()
  {
    sendTelemetry("IMU", "TELEMETRY", isTargetTelemetryEnabled(TARGET_IMU) ? "TRUE" : "FALSE");
  }

  void reportImuEnableStatus()
  {
    sendTelemetry("IMU", "ENABLE", imuState.enabled ? "TRUE" : "FALSE");
  }

  void reportImuAvailability()
  {
    sendTelemetry("IMU", "AVAILABLE", imuState.available ? "TRUE" : "FALSE");
  }

  void reportImuX()
  {
    sendTelemetryFloat("IMU", "X_MS2", imuState.available ? imuState.xMs2 : 0.0f, Config::Imu::accelerationDecimalPlaces);
  }

  void reportImuY()
  {
    sendTelemetryFloat("IMU", "Y_MS2", imuState.available ? imuState.yMs2 : 0.0f, Config::Imu::accelerationDecimalPlaces);
  }

  void reportImuZ()
  {
    sendTelemetryFloat("IMU", "Z_MS2", imuState.available ? imuState.zMs2 : 0.0f, Config::Imu::accelerationDecimalPlaces);
  }

  void reportImuGyroX()
  {
    sendTelemetryFloat("IMU", "GYRO_X_DPS", imuState.available ? imuState.gyroXDps : 0.0f, Config::Imu::gyroDecimalPlaces);
  }

  void reportImuGyroY()
  {
    sendTelemetryFloat("IMU", "GYRO_Y_DPS", imuState.available ? imuState.gyroYDps : 0.0f, Config::Imu::gyroDecimalPlaces);
  }

  void reportImuGyroZ()
  {
    sendTelemetryFloat("IMU", "GYRO_Z_DPS", imuState.available ? imuState.gyroZDps : 0.0f, Config::Imu::gyroDecimalPlaces);
  }
}

void setupImu()
{
  Wire.begin();

  imuState.enabled = Config::Imu::defaultEnabled;
  imuState.initialized = initializeSensor();
  imuState.available = false;
  imuState.lastReadMs = 0;
  imuState.hasAttemptedRead = false;
  clearImuValues();
}

void updateImu()
{
  if (!imuState.enabled)
  {
    imuState.available = false;
    clearImuValues();
    return;
  }

  if (!imuState.initialized)
  {
    imuState.initialized = initializeSensor();
    if (!imuState.initialized)
    {
      imuState.available = false;
      clearImuValues();
      return;
    }
  }

  const unsigned long nowMs = millis();
  if (!canReadNow(nowMs))
  {
    return;
  }

  imuState.lastReadMs = nowMs;
  imuState.hasAttemptedRead = true;

  uint8_t rawData[14];
  if (!readRegisters(kImuDataRegister, rawData, sizeof(rawData)))
  {
    imuState.available = false;
    clearImuValues();
    return;
  }

  const int16_t rawX = static_cast<int16_t>((static_cast<uint16_t>(rawData[0]) << 8) | rawData[1]);
  const int16_t rawY = static_cast<int16_t>((static_cast<uint16_t>(rawData[2]) << 8) | rawData[3]);
  const int16_t rawZ = static_cast<int16_t>((static_cast<uint16_t>(rawData[4]) << 8) | rawData[5]);
  const int16_t rawGyroX = static_cast<int16_t>((static_cast<uint16_t>(rawData[8]) << 8) | rawData[9]);
  const int16_t rawGyroY = static_cast<int16_t>((static_cast<uint16_t>(rawData[10]) << 8) | rawData[11]);
  const int16_t rawGyroZ = static_cast<int16_t>((static_cast<uint16_t>(rawData[12]) << 8) | rawData[13]);

  imuState.xMs2 = (static_cast<float>(rawX) / kImuScaleLsbPerG) * kStandardGravityMs2;
  imuState.yMs2 = (static_cast<float>(rawY) / kImuScaleLsbPerG) * kStandardGravityMs2;
  imuState.zMs2 = (static_cast<float>(rawZ) / kImuScaleLsbPerG) * kStandardGravityMs2;
  imuState.gyroXDps = static_cast<float>(rawGyroX) / kGyroScaleLsbPerDps;
  imuState.gyroYDps = static_cast<float>(rawGyroY) / kGyroScaleLsbPerDps;
  imuState.gyroZDps = static_cast<float>(rawGyroZ) / kGyroScaleLsbPerDps;
  imuState.available = true;
}

void reportImuStatus()
{
  updateImu();

  reportImuTelemetryStatus();
  reportImuEnableStatus();
  reportImuAvailability();
  reportImuX();
  reportImuY();
  reportImuZ();
  reportImuGyroX();
  reportImuGyroY();
  reportImuGyroZ();
}

void handleGetImu(const Command &cmd)
{
  if (cmd.value != VALUE_NONE)
  {
    sendError("BAD_FORMAT");
    return;
  }

  updateImu();

  switch (cmd.parameter)
  {
  case PARAM_NONE:
    reportImuStatus();
    break;

  case PARAM_TELEMETRY:
    reportImuTelemetryStatus();
    break;

  case PARAM_ENABLE:
    reportImuEnableStatus();
    break;

  case PARAM_AVAILABLE:
    reportImuAvailability();
    break;

  case PARAM_X_MS2:
    reportImuX();
    break;

  case PARAM_Y_MS2:
    reportImuY();
    break;

  case PARAM_Z_MS2:
    reportImuZ();
    break;

  case PARAM_GYRO_X_DPS:
    reportImuGyroX();
    break;

  case PARAM_GYRO_Y_DPS:
    reportImuGyroY();
    break;

  case PARAM_GYRO_Z_DPS:
    reportImuGyroZ();
    break;

  default:
    sendError("BAD_PARAMETER");
    break;
  }
}

void handleSetImu(const Command &cmd)
{
  switch (cmd.parameter)
  {
  case PARAM_ENABLE:
    if (cmd.value == VALUE_TRUE)
    {
      imuState.enabled = true;
      if (!imuState.initialized)
      {
        imuState.initialized = initializeSensor();
      }
      sendAck("IMU", "ENABLE");
    }
    else if (cmd.value == VALUE_FALSE)
    {
      imuState.enabled = false;
      imuState.available = false;
      clearImuValues();
      sendAck("IMU", "DISABLE");
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
