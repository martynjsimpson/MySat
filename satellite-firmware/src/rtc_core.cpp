#include "rtc.h"

#include <RTCZero.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "config.h"

static RTCZero rtc;
static bool clockSynchronized = false;
static const char *rtcSource = "NONE";

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

  const RtcDateTime initialDateTime = {
      Config::Rtc::initialYear,
      Config::Rtc::initialMonth,
      Config::Rtc::initialDay,
      Config::Rtc::initialHour,
      Config::Rtc::initialMinute,
      Config::Rtc::initialSecond};
  const RtcDateTime syncThreshold = {
      Config::Rtc::syncThresholdYear,
      Config::Rtc::syncThresholdMonth,
      Config::Rtc::syncThresholdDay,
      Config::Rtc::syncThresholdHour,
      Config::Rtc::syncThresholdMinute,
      Config::Rtc::syncThresholdSecond};
  // Synchronization is tracked heuristically: any RTC value at or beyond this
  // threshold is treated as having been set from a trusted source.

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

  unsigned long daysSinceUnixEpoch(const RtcDateTime &dt)
  {
    unsigned long days = 0;

    for (uint16_t year = 1970; year < dt.year; ++year)
    {
      days += isLeapYear(year) ? 366UL : 365UL;
    }

    for (uint8_t month = 1; month < dt.month; ++month)
    {
      days += daysInMonth(dt.year, month);
    }

    days += static_cast<unsigned long>(dt.day - 1);
    return days;
  }
}

void setupRtc()
{
  rtc.begin();
  setRtcDateTime(initialDateTime);
  // The MCU RTC has no persistent "time source" metadata, so startup always
  // begins from the configured baseline and an unsynchronized state.
  clockSynchronized = false;
  rtcSource = "NONE";
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

bool getCurrentTimeUnix(unsigned long &epochSeconds)
{
  const RtcDateTime dt = getRtcDateTime();

  if (!isValidDateTime(dt) || dt.year < 1970)
  {
    return false;
  }

  const unsigned long days = daysSinceUnixEpoch(dt);
  epochSeconds = (days * 86400UL) +
                 (static_cast<unsigned long>(dt.hour) * 3600UL) +
                 (static_cast<unsigned long>(dt.minute) * 60UL) +
                 static_cast<unsigned long>(dt.second);
  return true;
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

const char *getRtcSource()
{
  return rtcSource;
}

void setRtcSource(const char *source)
{
  rtcSource = (source == nullptr) ? "NONE" : source;
}
