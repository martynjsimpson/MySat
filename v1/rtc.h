#include <RTCZero.h>
RTCZero rtc;

void setupRtc() {
  rtc.begin();
  rtc.setY2kEpoch(0);
}

unsigned long getTimestamp() {
  return rtc.getY2kEpoch();
}