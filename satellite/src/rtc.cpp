#include "rtc.h"

#include <Arduino.h>
#include <RTCZero.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "gps.h"
#include "sender.h"
#include "telemetry.h"

static RTCZero rtc;
static bool clockSynchronized = false;

namespace
{
  struct RtcDateTime
  {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
  };

  const RtcDateTime initialDateTime = {2000, 1, 1, 0, 0, 0};
  const RtcDateTime syncThreshold = {2026, 3, 27, 0, 0, 0};
  bool autoSyncRtcFromGps = true;

  bool isLeapYear(uint16_t year)
  {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
  }

  uint8_t daysInMonth(uint16_t year, uint8_t month)
  {
    switch (month)
    {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
      return 31;
    case 4:
    case 6:
    case 9:
    case 11:
      return 30;
    case 2:
      return isLeapYear(year) ? 29 : 28;
    default:
      return 0;
    }
  }

  bool isValidDateTime(const RtcDateTime &dt)
  {
    if (dt.year < 2000 || dt.year > 2099)
    {
      return false;
    }

    if (dt.month < 1 || dt.month > 12)
    {
      return false;
    }

    if (dt.day < 1 || dt.day > daysInMonth(dt.year, dt.month))
    {
      return false;
    }

    return dt.hour <= 23 && dt.minute <= 59 && dt.second <= 59;
  }

  int compareDateTime(const RtcDateTime &lhs, const RtcDateTime &rhs)
  {
    if (lhs.year != rhs.year)
      return lhs.year < rhs.year ? -1 : 1;
    if (lhs.month != rhs.month)
      return lhs.month < rhs.month ? -1 : 1;
    if (lhs.day != rhs.day)
      return lhs.day < rhs.day ? -1 : 1;
    if (lhs.hour != rhs.hour)
      return lhs.hour < rhs.hour ? -1 : 1;
    if (lhs.minute != rhs.minute)
      return lhs.minute < rhs.minute ? -1 : 1;
    if (lhs.second != rhs.second)
      return lhs.second < rhs.second ? -1 : 1;
    return 0;
  }

  bool parseFixedDigits(const char *text, size_t start, size_t count, uint16_t &value)
  {
    value = 0;

    for (size_t i = 0; i < count; ++i)
    {
      const char c = text[start + i];
      if (c < '0' || c > '9')
      {
        return false;
      }

      value = static_cast<uint16_t>(value * 10 + (c - '0'));
    }

    return true;
  }

  bool parseIsoTimestamp(const char *isoTimestamp, RtcDateTime &dt)
  {
    if (isoTimestamp == nullptr || strlen(isoTimestamp) != 20)
    {
      return false;
    }

    if (isoTimestamp[4] != '-' ||
        isoTimestamp[7] != '-' ||
        isoTimestamp[10] != 'T' ||
        isoTimestamp[13] != ':' ||
        isoTimestamp[16] != ':' ||
        isoTimestamp[19] != 'Z')
    {
      return false;
    }

    uint16_t year = 0;
    uint16_t month = 0;
    uint16_t day = 0;
    uint16_t hour = 0;
    uint16_t minute = 0;
    uint16_t second = 0;

    if (!parseFixedDigits(isoTimestamp, 0, 4, year) ||
        !parseFixedDigits(isoTimestamp, 5, 2, month) ||
        !parseFixedDigits(isoTimestamp, 8, 2, day) ||
        !parseFixedDigits(isoTimestamp, 11, 2, hour) ||
        !parseFixedDigits(isoTimestamp, 14, 2, minute) ||
        !parseFixedDigits(isoTimestamp, 17, 2, second))
    {
      return false;
    }

    dt.year = year;
    dt.month = static_cast<uint8_t>(month);
    dt.day = static_cast<uint8_t>(day);
    dt.hour = static_cast<uint8_t>(hour);
    dt.minute = static_cast<uint8_t>(minute);
    dt.second = static_cast<uint8_t>(second);

    return isValidDateTime(dt);
  }

  void setRtcDateTime(const RtcDateTime &dt)
  {
    rtc.setTime(dt.hour, dt.minute, dt.second);
    rtc.setDate(dt.day, dt.month, dt.year - 2000);
  }

  RtcDateTime getRtcDateTime()
  {
    RtcDateTime dt{};
    dt.year = static_cast<uint16_t>(2000 + rtc.getYear());
    dt.month = rtc.getMonth();
    dt.day = rtc.getDay();
    dt.hour = rtc.getHours();
    dt.minute = rtc.getMinutes();
    dt.second = rtc.getSeconds();
    return dt;
  }

  void emitRtcSyncTelemetryIfTransitioned(bool wasClockSynchronized)
  {
    if (!wasClockSynchronized && clockSynchronized)
    {
      reportRtcSyncStatus();
    }
  }
} // namespace

void setupRtc()
{
  rtc.begin();
  setRtcDateTime(initialDateTime);
  clockSynchronized = false;
}

unsigned long getUptimeSeconds()
{
  return millis() / 1000UL;
}

bool getCurrentTimestampIso(char *buffer, size_t bufferSize)
{
  if (buffer == nullptr || bufferSize < 21)
  {
    return false;
  }

  const RtcDateTime dt = getRtcDateTime();
  const int written = snprintf(
      buffer,
      bufferSize,
      "%04u-%02u-%02uT%02u:%02u:%02uZ",
      dt.year,
      dt.month,
      dt.day,
      dt.hour,
      dt.minute,
      dt.second);

  return written > 0 && static_cast<size_t>(written) < bufferSize;
}

bool setCurrentTimeIso(const char *isoTimestamp)
{
  RtcDateTime dt{};
  if (!parseIsoTimestamp(isoTimestamp, dt))
  {
    return false;
  }

  setRtcDateTime(dt);
  clockSynchronized = compareDateTime(dt, syncThreshold) >= 0;
  return true;
}

bool setCurrentTimeUnix(unsigned long epochSeconds)
{
  const time_t rawTime = static_cast<time_t>(epochSeconds);
  const struct tm *utc = gmtime(&rawTime);
  if (utc == nullptr)
  {
    return false;
  }

  RtcDateTime dt{};
  dt.year = static_cast<uint16_t>(utc->tm_year + 1900);
  dt.month = static_cast<uint8_t>(utc->tm_mon + 1);
  dt.day = static_cast<uint8_t>(utc->tm_mday);
  dt.hour = static_cast<uint8_t>(utc->tm_hour);
  dt.minute = static_cast<uint8_t>(utc->tm_min);
  dt.second = static_cast<uint8_t>(utc->tm_sec);

  if (!isValidDateTime(dt))
  {
    return false;
  }

  setRtcDateTime(dt);
  clockSynchronized = compareDateTime(dt, syncThreshold) >= 0;
  return true;
}

bool isClockSynchronized()
{
  return clockSynchronized;
}

void handleRtcAutoSync()
{
  if (!autoSyncRtcFromGps || clockSynchronized)
  {
    return;
  }

  unsigned long gpsEpochSeconds = 0;
  if (!getGpsTimeUnix(gpsEpochSeconds))
  {
    return;
  }

  const bool wasClockSynchronized = clockSynchronized;
  if (setCurrentTimeUnix(gpsEpochSeconds))
  {
    emitRtcSyncTelemetryIfTransitioned(wasClockSynchronized);
  }
}

void reportRtcTelemetryStatus()
{
  sendTelemetry("RTC", "TELEMETRY", isTargetTelemetryEnabled(TARGET_RTC) ? "TRUE" : "FALSE");
}

void reportRtcCurrentTime()
{
  char timestamp[21];
  if (!getCurrentTimestampIso(timestamp, sizeof(timestamp)))
  {
    sendError("RTC_READ_FAILED");
    return;
  }

  sendTelemetry("RTC", "CURRENT_TIME", timestamp);
}

void reportRtcSyncStatus()
{
  sendTelemetry("RTC", "SYNC", clockSynchronized ? "TRUE" : "FALSE");
}

void reportRtcStatus()
{
  reportRtcTelemetryStatus();
  reportRtcCurrentTime();
  reportRtcSyncStatus();
}

void handleGetRtc(const Command &cmd)
{
  if (cmd.value != VALUE_NONE)
  {
    sendError("BAD_FORMAT");
    return;
  }

  switch (cmd.parameter)
  {
  case PARAM_NONE:
    reportRtcStatus();
    break;

  case PARAM_CURRENT_TIME:
    reportRtcCurrentTime();
    break;

  case PARAM_TELEMETRY:
    reportRtcTelemetryStatus();
    break;

  case PARAM_SYNC:
    reportRtcSyncStatus();
    break;

  default:
    sendError("BAD_PARAMETER");
    break;
  }
}

void handleSetRtc(const Command &cmd)
{
  switch (cmd.parameter)
  {
  case PARAM_CURRENT_TIME:
  {
    const bool wasClockSynchronized = clockSynchronized;
    if (cmd.rawValueToken == nullptr || !setCurrentTimeIso(cmd.rawValueToken))
    {
      sendError("BAD_VALUE");
      return;
    }

    emitRtcSyncTelemetryIfTransitioned(wasClockSynchronized);
    sendAck("RTC", "CURRENT_TIME");
    break;
  }

  case PARAM_SYNC:
  {
    const bool wasClockSynchronized = clockSynchronized;
    if (cmd.rawValueToken == nullptr || strcmp(cmd.rawValueToken, "GPS") != 0)
    {
      sendError("BAD_VALUE");
      return;
    }

    unsigned long gpsEpochSeconds = 0;
    if (!getGpsTimeUnix(gpsEpochSeconds) || !setCurrentTimeUnix(gpsEpochSeconds))
    {
      sendError("GPS_TIME_UNAVAILABLE");
      return;
    }

    emitRtcSyncTelemetryIfTransitioned(wasClockSynchronized);
    sendAck("RTC", "SYNC");
    break;
  }

  default:
    sendError("BAD_PARAMETER");
    break;
  }
}
