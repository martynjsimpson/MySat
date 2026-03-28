#include "transport.h"

#include <Arduino.h>

namespace
{
  constexpr unsigned long kSerialBaudRate = 115200;
}

void setupTransport()
{
  // Today the transport abstraction is serial-backed; future transports can
  // provide the same API without changing protocol or sender code.
  Serial.begin(kSerialBaudRate);
}

int transportAvailable()
{
  return Serial.available();
}

int transportRead()
{
  return Serial.read();
}

void transportWrite(const char *text)
{
  Serial.print(text);
}

void transportWrite(char value)
{
  Serial.print(value);
}

void transportWrite(unsigned long value)
{
  Serial.print(value);
}

void transportWrite(float value, int decimals)
{
  Serial.print(value, decimals);
}

void transportWriteLine()
{
  Serial.println();
}

void transportFlush()
{
  Serial.flush();
}
