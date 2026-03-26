#ifndef RTC_HELPER_H
#define RTC_HELPER_H

#include <RTCZero.h>

RTCZero rtc;

void setupRtc() {
  rtc.begin();
  rtc.setY2kEpoch(0);
}

unsigned long getTimestamp() {
  return rtc.getY2kEpoch();
}

#endif
