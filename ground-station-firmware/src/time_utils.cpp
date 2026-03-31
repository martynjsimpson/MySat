#include "time_utils.h"

#include <string.h>

namespace
{
  bool isLeapYear(int year)
  {
    return ((year % 4) == 0 && (year % 100) != 0) || ((year % 400) == 0);
  }

  uint32_t daysFromCivil(int year, unsigned month, unsigned day)
  {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return static_cast<uint32_t>(era * 146097 + static_cast<int>(doe) - 719468);
  }

  void civilFromDays(int64_t z, int &year, unsigned &month, unsigned &day)
  {
    z += 719468;
    const int era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    year = static_cast<int>(yoe) + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    day = doy - (153 * mp + 2) / 5 + 1;
    month = mp + (mp < 10 ? 3 : -9);
    year += (month <= 2);
  }

  bool parseTwoDigits(const char *text, int &value)
  {
    if (text[0] < '0' || text[0] > '9' || text[1] < '0' || text[1] > '9')
    {
      return false;
    }

    value = (text[0] - '0') * 10 + (text[1] - '0');
    return true;
  }

  bool parseFourDigits(const char *text, int &value)
  {
    value = 0;
    for (size_t index = 0; index < 4; ++index)
    {
      if (text[index] < '0' || text[index] > '9')
      {
        return false;
      }

      value = (value * 10) + (text[index] - '0');
    }

    return true;
  }
} // namespace

bool parseIsoTimestamp(const char *timestamp, uint32_t &outEpochSeconds)
{
  if (timestamp == nullptr || strlen(timestamp) != 20)
  {
    return false;
  }

  if (timestamp[4] != '-' ||
      timestamp[7] != '-' ||
      timestamp[10] != 'T' ||
      timestamp[13] != ':' ||
      timestamp[16] != ':' ||
      timestamp[19] != 'Z')
  {
    return false;
  }

  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;

  if (!parseFourDigits(timestamp, year) ||
      !parseTwoDigits(timestamp + 5, month) ||
      !parseTwoDigits(timestamp + 8, day) ||
      !parseTwoDigits(timestamp + 11, hour) ||
      !parseTwoDigits(timestamp + 14, minute) ||
      !parseTwoDigits(timestamp + 17, second))
  {
    return false;
  }

  if (month < 1 || month > 12 || day < 1 || hour > 23 || minute > 59 || second > 59)
  {
    return false;
  }

  static const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int maxDay = daysInMonth[month - 1];
  if (month == 2 && isLeapYear(year))
  {
    maxDay = 29;
  }

  if (day > maxDay)
  {
    return false;
  }

  const uint32_t days = daysFromCivil(year, static_cast<unsigned>(month), static_cast<unsigned>(day));
  outEpochSeconds = (days * 86400UL) +
                    (static_cast<uint32_t>(hour) * 3600UL) +
                    (static_cast<uint32_t>(minute) * 60UL) +
                    static_cast<uint32_t>(second);
  return true;
}

void formatIsoTimestamp(uint32_t epochSeconds, char *buffer, size_t bufferSize)
{
  if (buffer == nullptr || bufferSize < 21)
  {
    return;
  }

  const uint32_t days = epochSeconds / 86400UL;
  uint32_t secondsOfDay = epochSeconds % 86400UL;

  int year = 0;
  unsigned month = 0;
  unsigned day = 0;
  civilFromDays(days, year, month, day);

  const unsigned hour = secondsOfDay / 3600UL;
  secondsOfDay %= 3600UL;
  const unsigned minute = secondsOfDay / 60UL;
  const unsigned second = secondsOfDay % 60UL;

  buffer[0] = static_cast<char>('0' + ((year / 1000) % 10));
  buffer[1] = static_cast<char>('0' + ((year / 100) % 10));
  buffer[2] = static_cast<char>('0' + ((year / 10) % 10));
  buffer[3] = static_cast<char>('0' + (year % 10));
  buffer[4] = '-';
  buffer[5] = static_cast<char>('0' + ((month / 10) % 10));
  buffer[6] = static_cast<char>('0' + (month % 10));
  buffer[7] = '-';
  buffer[8] = static_cast<char>('0' + ((day / 10) % 10));
  buffer[9] = static_cast<char>('0' + (day % 10));
  buffer[10] = 'T';
  buffer[11] = static_cast<char>('0' + ((hour / 10) % 10));
  buffer[12] = static_cast<char>('0' + (hour % 10));
  buffer[13] = ':';
  buffer[14] = static_cast<char>('0' + ((minute / 10) % 10));
  buffer[15] = static_cast<char>('0' + (minute % 10));
  buffer[16] = ':';
  buffer[17] = static_cast<char>('0' + ((second / 10) % 10));
  buffer[18] = static_cast<char>('0' + (second % 10));
  buffer[19] = 'Z';
  buffer[20] = '\0';
}
