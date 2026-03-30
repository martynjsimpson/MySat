#include "imu.h"

#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#include "config.h"
#include "sender.h"
#include "telemetry.h"

namespace
{
  constexpr uint8_t kMpuPowerManagementRegister = 0x6B;
  constexpr uint8_t kMpuWhoAmIRegister = 0x75;
  constexpr uint8_t kMpuDataRegister = 0x3B;
  constexpr uint8_t kMpuExpectedWhoAmI = 0x68;
  constexpr uint8_t kMpuAlternateWhoAmI = 0x69;
  constexpr uint8_t kQmcDataRegister = 0x00;
  constexpr uint8_t kQmcControl1Register = 0x09;
  constexpr uint8_t kQmcControl2Register = 0x0A;
  constexpr uint8_t kQmcSetResetPeriodRegister = 0x0B;
  constexpr uint8_t kQmcChipIdRegister = 0x0D;
  constexpr uint8_t kQmcExpectedChipId = 0xFF;
  constexpr uint8_t kQmcContinuousMode = 0x15;
  constexpr uint8_t kQmcSoftReset = 0x80;
  constexpr uint8_t kQmcRecommendedSetResetPeriod = 0x01;
  constexpr float kStandardGravityMs2 = 9.80665f;
  constexpr float kImuScaleLsbPerG = 16384.0f;
  constexpr float kGyroScaleLsbPerDps = 131.0f;
  constexpr float kQmcScaleLsbPerUt = 30.0f;
  constexpr float kRadiansToDegrees = 57.2957795f;

  struct ImuState
  {
    bool enabled;
    bool motionInitialized;
    bool magnetometerInitialized;
    bool available;
    bool magnetometerAvailable;
    float xMs2;
    float yMs2;
    float zMs2;
    float gyroXDps;
    float gyroYDps;
    float gyroZDps;
    float magXUt;
    float magYUt;
    float magZUt;
    float headingDeg;
    unsigned long lastReadMs;
    bool hasAttemptedRead;
  };

  ImuState imuState{};

  bool writeRegister(uint8_t address, uint8_t reg, uint8_t value)
  {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
  }

  bool readRegisters(uint8_t address, uint8_t startRegister, uint8_t *buffer, size_t length)
  {
    Wire.beginTransmission(address);
    Wire.write(startRegister);
    if (Wire.endTransmission(false) != 0)
    {
      return false;
    }

    const size_t bytesRead = Wire.requestFrom(static_cast<int>(address), static_cast<int>(length));
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
    imuState.magXUt = 0.0f;
    imuState.magYUt = 0.0f;
    imuState.magZUt = 0.0f;
    imuState.headingDeg = 0.0f;
  }

  bool canReadNow(unsigned long nowMs)
  {
    return !imuState.hasAttemptedRead ||
           (nowMs - imuState.lastReadMs) >= Config::Imu::minReadIntervalMs;
  }

  void clearMagnetometerValues()
  {
    imuState.magXUt = 0.0f;
    imuState.magYUt = 0.0f;
    imuState.magZUt = 0.0f;
    imuState.headingDeg = 0.0f;
  }

  bool initializeMotionSensor()
  {
    uint8_t whoAmI = 0;
    if (!readRegisters(Config::Imu::mpuI2cAddress, kMpuWhoAmIRegister, &whoAmI, 1))
    {
      return false;
    }

    if (whoAmI != kMpuExpectedWhoAmI && whoAmI != kMpuAlternateWhoAmI)
    {
      return false;
    }

    return writeRegister(Config::Imu::mpuI2cAddress, kMpuPowerManagementRegister, 0x00);
  }

  bool initializeMagnetometer()
  {
    uint8_t chipId = 0;
    if (!readRegisters(Config::Imu::magnetometerI2cAddress, kQmcChipIdRegister, &chipId, 1))
    {
      return false;
    }

    if (chipId != kQmcExpectedChipId)
    {
      return false;
    }

    if (!writeRegister(Config::Imu::magnetometerI2cAddress, kQmcControl2Register, kQmcSoftReset))
    {
      return false;
    }

    delay(1);

    if (!writeRegister(Config::Imu::magnetometerI2cAddress, kQmcSetResetPeriodRegister, kQmcRecommendedSetResetPeriod))
    {
      return false;
    }

    return writeRegister(Config::Imu::magnetometerI2cAddress, kQmcControl1Register, kQmcContinuousMode);
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

  void reportImuMagX()
  {
    sendTelemetryFloat("IMU", "MAG_X_UT", imuState.magnetometerAvailable ? imuState.magXUt : 0.0f, Config::Imu::magneticDecimalPlaces);
  }

  void reportImuMagY()
  {
    sendTelemetryFloat("IMU", "MAG_Y_UT", imuState.magnetometerAvailable ? imuState.magYUt : 0.0f, Config::Imu::magneticDecimalPlaces);
  }

  void reportImuMagZ()
  {
    sendTelemetryFloat("IMU", "MAG_Z_UT", imuState.magnetometerAvailable ? imuState.magZUt : 0.0f, Config::Imu::magneticDecimalPlaces);
  }

  void reportImuHeading()
  {
    sendTelemetryFloat("IMU", "HEADING_DEG", imuState.magnetometerAvailable ? imuState.headingDeg : 0.0f, Config::Imu::headingDecimalPlaces);
  }
}

void setupImu()
{
  Wire.begin();

  imuState.enabled = Config::Imu::defaultEnabled;
  imuState.motionInitialized = initializeMotionSensor();
  imuState.magnetometerInitialized = initializeMagnetometer();
  imuState.available = false;
  imuState.magnetometerAvailable = false;
  imuState.lastReadMs = 0;
  imuState.hasAttemptedRead = false;
  clearImuValues();
}

void updateImu()
{
  if (!imuState.enabled)
  {
    imuState.available = false;
    imuState.magnetometerAvailable = false;
    clearImuValues();
    return;
  }

  if (!imuState.motionInitialized)
  {
    imuState.motionInitialized = initializeMotionSensor();
    if (!imuState.motionInitialized)
    {
      imuState.available = false;
      imuState.magnetometerAvailable = false;
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
  if (!readRegisters(Config::Imu::mpuI2cAddress, kMpuDataRegister, rawData, sizeof(rawData)))
  {
    imuState.available = false;
    imuState.magnetometerAvailable = false;
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

  if (!imuState.magnetometerInitialized)
  {
    imuState.magnetometerInitialized = initializeMagnetometer();
  }

  if (!imuState.magnetometerInitialized)
  {
    imuState.magnetometerAvailable = false;
    clearMagnetometerValues();
    return;
  }

  uint8_t rawMagData[6];
  if (!readRegisters(Config::Imu::magnetometerI2cAddress, kQmcDataRegister, rawMagData, sizeof(rawMagData)))
  {
    imuState.magnetometerAvailable = false;
    clearMagnetometerValues();
    return;
  }

  const int16_t rawMagX = static_cast<int16_t>((static_cast<uint16_t>(rawMagData[1]) << 8) | rawMagData[0]);
  const int16_t rawMagY = static_cast<int16_t>((static_cast<uint16_t>(rawMagData[3]) << 8) | rawMagData[2]);
  const int16_t rawMagZ = static_cast<int16_t>((static_cast<uint16_t>(rawMagData[5]) << 8) | rawMagData[4]);

  imuState.magXUt = static_cast<float>(rawMagX) / kQmcScaleLsbPerUt;
  imuState.magYUt = static_cast<float>(rawMagY) / kQmcScaleLsbPerUt;
  imuState.magZUt = static_cast<float>(rawMagZ) / kQmcScaleLsbPerUt;

  float headingDeg = atan2f(imuState.magYUt, imuState.magXUt) * kRadiansToDegrees;
  if (headingDeg < 0.0f)
  {
    headingDeg += 360.0f;
  }

  imuState.headingDeg = headingDeg;
  imuState.magnetometerAvailable = true;
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
  reportImuMagX();
  reportImuMagY();
  reportImuMagZ();
  reportImuHeading();
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

  case PARAM_MAG_X_UT:
    reportImuMagX();
    break;

  case PARAM_MAG_Y_UT:
    reportImuMagY();
    break;

  case PARAM_MAG_Z_UT:
    reportImuMagZ();
    break;

  case PARAM_HEADING_DEG:
    reportImuHeading();
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
      if (!imuState.motionInitialized)
      {
        imuState.motionInitialized = initializeMotionSensor();
      }
      if (!imuState.magnetometerInitialized)
      {
        imuState.magnetometerInitialized = initializeMagnetometer();
      }
      sendAck("IMU", "ENABLE");
    }
    else if (cmd.value == VALUE_FALSE)
    {
      imuState.enabled = false;
      imuState.available = false;
      imuState.magnetometerAvailable = false;
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

ImuReadings getImuReadings()
{
  updateImu();

  return ImuReadings{
      imuState.enabled,
      imuState.available,
      imuState.xMs2,
      imuState.yMs2,
      imuState.zMs2,
      imuState.gyroXDps,
      imuState.gyroYDps,
      imuState.gyroZDps,
      imuState.magXUt,
      imuState.magYUt,
      imuState.magZUt,
      imuState.headingDeg};
}
